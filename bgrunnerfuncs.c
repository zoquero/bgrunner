/*
 * Background jobs runner helper functions
 * 
 * Sources: https://github.com/zoquero/bgrunner/
 * 
 * @since 20161208
 * @author zoquero@gmail.com
 */

#include <stdio.h>        // printf, fgetc
#include <stdlib.h>       // exit
#include <regex.h>
#include <string.h>       // strlen
#include <sys/types.h>    // pid_t , kill
#include <unistd.h>       // pid_t
#include <sys/wait.h>     // waitpid
#include <pthread.h>      // pthread_create ...
#include <unistd.h>       // usleep
#include <sys/time.h>     // gettimeofday
#include <signal.h>       // kill

#include "bgrunner.h"

#define SLEEP_TIME 10000            // in microseconds
#define US_TO_SHOW_ON_DEBUG 1000000 // 1 second
#define BUFSIZE 40960


/**
  * Print to stdout with UTC timestamp
  * @arg s string to write
  */
void tPrint (char *s) {
  time_t     rawtime;
  struct tm *info;
  char       buffer[80];
  struct tm  result;
  
  time(&rawtime);
  info=gmtime_r(&rawtime, &result); // _r for thread safe
  strftime(buffer,80, "%Y-%m-%d %H:%M:%SZ", info);
  printf("%s: %s\n", buffer, s);
}


/*
 * Difference between 2 points in time.
 *
 * @arg startup time
 * @arg startup time
 * @return difference in miliseconds
 */
double timeval_diff(struct timeval *a, struct timeval *b) {
  return 1000 * ((double)(a->tv_sec + (double)a->tv_usec/1000000) - (double)(b->tv_sec + (double)b->tv_usec/1000000));
}


void waitForJobs(bgjob *jobs, unsigned int numJobs, int verbose) {
  int finishedJobs;
  pid_t w;
  int status;
  unsigned int sleepTime = SLEEP_TIME;
  unsigned int z = 0;
  char MSGBUFF[BUFSIZE];

  if(verbose) {
    sprintf(MSGBUFF, "Let's wait for the jobs with a sleepTime of %u us", sleepTime);
    tPrint(MSGBUFF);
  }
  for(;;) {
    finishedJobs = 0;
    for(int i = 0; i < numJobs; i++) {
      if(jobs[i].state == STARTED) {
        w = waitpid(jobs[i].pid, &status, WNOHANG);
        if(w == jobs[i].pid) {
          jobs[i].state = FINISHED;
          finishedJobs++;
          if(verbose) {
            sprintf(MSGBUFF, "Job [%s] just finished", jobs[i].alias);
            tPrint(MSGBUFF);
          }
        }
        else if(w == -1) {
          fprintf (stderr, "Bug: job [%s] with pid [%d] has already finished\n", jobs[i].alias, jobs[i].pid);
          exit(1);
        }
        else if(w != 0) {  // 0 is for already running child
          fprintf (stderr, "Unknown state for job [%s]: %d\n", jobs[i].alias, jobs[i].state);
          exit(1);
        }
        else {  // already running
          // Timeout just applies if maxDurationMS is not 0
          if(jobs[i].maxDurationMS != 0) {
            struct timeval now;
            gettimeofday(&now, NULL);
            if(timeval_diff(&now, &(jobs[i].startupTime)) > jobs[i].maxDurationMS + jobs[i].startAfterMS) {
              sprintf(MSGBUFF, "Job [%s] has been running more than %u ms. Let's kill it", jobs[i].alias, jobs[i].maxDurationMS);
              tPrint(MSGBUFF);
              kill(jobs[i].pid, SIGKILL);
            }
          }
        }
      }
      else if(jobs[i].state == FINISHED) {
        finishedJobs++;
      }
    }
    if(finishedJobs == numJobs) {
      if(verbose) { sprintf(MSGBUFF, "All jobs finished"); tPrint(MSGBUFF); }
      return;
    }

    if(verbose > 1)
      if(z * sleepTime >= US_TO_SHOW_ON_DEBUG) {
        z = 0;
        sprintf(MSGBUFF, "%d finished jobs", finishedJobs);
        tPrint(MSGBUFF);
      }
    usleep(sleepTime);
    z++;
  }
}


/**
  * Returns the number of lines on a *NIX text file, empty or not.
  */
unsigned int countLines(char *filename) {
  FILE* myFile = fopen(filename, "r");
  if(myFile == NULL) {
    fprintf (stderr, "Can't read the job descriptor %s\n", filename);
    exit(1);
  }
  int ch, nol = 0;
  
  do {
    ch = fgetc(myFile);
    if(ch == '\n')
      nol++;
  } while (ch != EOF);
  
  // last line doesn't end with a new line,
  // but there has to be a line at least before the last line
  if(ch != '\n' && nol != 0) 
      nol++;
  fclose(myFile);
  return nol;
}


void printJobShort(bgjob *b) {
  printf("BackGround job with alias=[%s] that after %ums will run [%s] for up to %ums\n", b->alias, b->startAfterMS, b->command, b->maxDurationMS);
}


void printJob(bgjob *b) {
  printf("BackGround job with alias=[%s] that after %ums will run [%s] for up to %ums, with pid [%d] and state [%d]\n", b->alias, b->startAfterMS, b->command, b->maxDurationMS, b->pid, b->state);
}

void printJobFull(bgjob *b) {
  printf("BackGround job with id=[%d], alias=[%s], startAfterMS=[%u], maxDurationMS=[%u], command=[%s], pid=[%u], state=[%d], startupTime=[?], envp=[?], verbose=[%d]\n", b->id, b->alias, b->startAfterMS, b->maxDurationMS, b->command, b->pid, b->state, /* b->startupTime, b->envp, */ b->verbose);
}



bgjob *loadJobs(char *filename, unsigned int *numJobs, char *envp[], int verbose) {
  char line[MAX_ALIAS_LEN+PATH_MAX+50];
  unsigned int numLines;
  unsigned int id = 0;
  char alias[MAX_ALIAS_LEN];
  unsigned int startAfterMS;
  unsigned int maxDurationMS;
  char command[PATH_MAX];
  bgjob *b;
  char *regexString = "([^;]+);([^;]+);([^;]+);([^;]+)";
  regex_t regexCompiled;
  size_t maxGroups = 5;
  char  *end;
  float  num;
  unsigned int g = 0;
  char sourceCopy[MAX_ALIAS_LEN+PATH_MAX+50 + 1];
  regmatch_t groupArray[maxGroups];
  char MSGBUFF[BUFSIZE];
  bgjob* jobs;

  if(verbose > 1) {
    sprintf(MSGBUFF, "Using regexp [%s] when parsing the job descriptor [%s]", regexString, filename);
    tPrint(MSGBUFF);
  }

  // example about C regexp: http://stackoverflow.com/a/11864144/939015
  // build regex
  if (regcomp(&regexCompiled, regexString, REG_EXTENDED)) {
    fprintf(stderr, "Can't compile regular expression [%s]\n", regexString);
    exit(1);
  }

  // read length of file and alloc bgjob array
  numLines = countLines(filename);
  if(verbose > 1) {
    sprintf(MSGBUFF, "%u lines on job descriptor %s", numLines, filename);
    tPrint(MSGBUFF);
  }
  jobs = (bgjob*) malloc(numLines * sizeof(bgjob));

  FILE* myFile = fopen(filename, "r");

  while(fscanf(myFile, "%s\n", line) == 1) {
    // headers, comments
    if(*line == '#')
      continue;

    if (regexec(&regexCompiled, line, maxGroups, groupArray, 0) == 0) {
      *alias             = '\0';
      startAfterMS  = 0;
      maxDurationMS = 0;
      *command           = '\0';
      for (g = 0; g < maxGroups; g++) {
        if (groupArray[g].rm_so == (size_t)-1)
          break;  // No more groups
  
        strcpy(sourceCopy, line);
        sourceCopy[groupArray[g].rm_eo] = 0;
  
        switch (g) {
        case 1:
          strcpy(alias, sourceCopy + groupArray[g].rm_so);
          if(*alias == '\0') {
            fprintf(stderr, "Can't parse alias in line [%s] on descriptor %s\n", line, filename);
            exit(1);
          }
          break;
        case 2:
          if(sscanf(sourceCopy + groupArray[g].rm_so, "%u", &startAfterMS) != 1) {
            fprintf(stderr, "Can't read startAfterMS in line [%s] on descriptor %s\n", line, filename);
            exit(1);
          }
          break;
        case 3:
          if(sscanf(sourceCopy + groupArray[g].rm_so, "%u", &maxDurationMS) != 1) {
            fprintf(stderr, "Can't read maxDurationMS in line [%s] on descriptor %s\n", line, filename);
            exit(1);
          }
          break;
        case 4:
          strcpy(command, sourceCopy + groupArray[g].rm_so);
          if(*command == '\0') {
            fprintf(stderr, "Can't parse command in line [%s] on descriptor %s\n", line, filename);
            exit(1);
          }
          break;
        }
      }
      b                = jobs + id;
      b->id            = id;
      strcpy(b->alias, alias);
      b->startAfterMS  = startAfterMS;
      b->maxDurationMS = maxDurationMS;
      strcpy(b->command, command);
      b->state         = UNSTARTED;
      b->verbose       = verbose;
      b->envp          = envp;
      id++;
    }
  }
  
  fclose(myFile);
  *numJobs = id;
  return jobs;
}


void *execStartupRoutine (void *arg) {
  char MSGBUFF[BUFSIZE];
  bgjob *job = (bgjob *) arg;

  if(job->verbose > 1) {
    sprintf(MSGBUFF, "Thread for job [%s]: forking from pid %d to exec the job", job->alias, getpid());
    tPrint(MSGBUFF);
  }
  pid_t pid;
  pid = fork();

  if (pid == -1) {
    /* error */
    fprintf(stderr, "Can't fork\n");
    exit(1);
  }
  else if (pid == 0) {
    /* child */
    if(job->startAfterMS > 0) {
      if(job->verbose > 1) {
        sprintf(MSGBUFF, "Thread for job [%s]: child process sleeps %dms", job->alias, job->startAfterMS);
        tPrint(MSGBUFF);
      }
      usleep(1000 * job->startAfterMS);
      if(job->verbose > 1) {
        sprintf(MSGBUFF, "Thread for job [%s]: child process awakes", job->alias);
        tPrint(MSGBUFF);
      }
    }

    //...
    if(job->verbose > 0) {
      sprintf(MSGBUFF, "Thread for job [%s]: child process executing [%s]", job->alias, job->command);
      tPrint(MSGBUFF);
    }
    char *myArgs[2] = { job->command, NULL };
    execve(job->command, myArgs, job->envp);

    /* execve() just returns on error */
    fprintf(stderr, "Error calling execve from the child process\n");
    exit(1);
  }
  else {
    /* parent */
    struct timeval now;
    gettimeofday(&now, NULL);

    job->pid         = pid;
    job->state       = STARTED;
    job->startupTime = now;

//  if(job->verbose > 1) printf("Thread for job [%s]: parent after exec\n", job->alias);
  }
}


void launchJobs(char *filename, char *envp[], int verbose) {
  unsigned int numJobs;
  bgjob* jobs;
  pthread_t *threads;

  jobs = loadJobs(filename, &numJobs, envp, verbose);
  threads = (pthread_t *) malloc(numJobs * sizeof(pthread_t));

  if(verbose) {
    printf("%u jobs described on %s:\n", numJobs, filename);
    for(int i = 0; i < numJobs; i++) {
      printf("* "); printJobShort(jobs+i);
    }
  }

  for(int i = 0; i < numJobs; i++) {
    if(verbose > 1) printf("Creating the launcher thread for the job [%s]\n", (*((bgjob *)(jobs+i))).alias);

    if(pthread_create(threads+i, NULL, execStartupRoutine, (void *) &(jobs[i]))) {
      fprintf(stderr, "Can't create the thread\n");
      exit(1);
    }
  }

  waitForJobs(jobs, numJobs, verbose);
  free(threads);
  free(jobs);
}

