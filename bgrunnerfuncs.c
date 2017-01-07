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
#include <unistd.h>       // pid_t , usleep
#include <sys/wait.h>     // waitpid
#include <pthread.h>      // pthread_create ...
#include <sys/time.h>     // gettimeofday
#include <signal.h>       // kill
#include <fcntl.h>        // open

#include "bgrunner.h"

#define SLEEP_TIME 10000            // in microseconds
#define US_TO_SHOW_ON_DEBUG 1000000 // 1 second
#define BUFSIZE 1024

// pthread_mutex_t mi_mutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_lock(&mi_mutex);
// pthread_mutex_unlock(&mi_mutex);


/**
  * Print to stdout with UTC timestamp
  * @arg s string to write
  */
void tPrint (char *s) {
  
  time_t     rawtime;
  char       buffer[80];
  struct tm  result;
  time(&rawtime);
  if(gmtime_r(&rawtime, &result) != NULL) {    // _r for thread safeness
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%SZ", &result);
    printf("%s: %s\n", buffer, s);
  }
/*
  A problem: gmtime_r and ctime sometimes stops the program's execution
  printf("log: %s\n", s);
*/
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
            sprintf(MSGBUFF, "Job [%s] finished", jobs[i].alias);
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
            if(timeval_diff(&now, &(jobs[i].startupTime)) > jobs[i].maxDurationMS + jobs[i].startAfterMS ) {
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
//printf("BackGround job with alias=[%s] that will run [%s] for up to %ums\n", b->alias, b->command, b->maxDurationMS);
}


void printJob(bgjob *b) {
  printf("BackGround job with alias=[%s] that after %ums will run [%s] for up to %ums, with pid [%d] and state [%d]\n", b->alias, b->startAfterMS, b->command, b->maxDurationMS, b->pid, b->state);
//printf("BackGround job with alias=[%s] that will run [%s] for up to %ums, with pid [%d] and state [%d]\n", b->alias, b->command, b->maxDurationMS, b->pid, b->state);
}

void printJobFull(bgjob *b) {
  printf("BackGround job with id=[%d], alias=[%s], startAfterMS=[%u], maxDurationMS=[%u], command=[%s], pid=[%u], state=[%d], startupTime=[?], envp=[?], verbose=[%d]\n", b->id, b->alias, b->startAfterMS, b->maxDurationMS, b->command, b->pid, b->state, /* b->startupTime, b->envp, */ b->verbose);
//printf("BackGround job with id=[%d], alias=[%s], maxDurationMS=[%u], command=[%s], pid=[%u], state=[%d], startupTime=[?], envp=[?], verbose=[%d]\n", b->id, b->alias, b->maxDurationMS, b->command, b->pid, b->state, /* b->startupTime, b->envp, */ b->verbose);
}



bgjob *loadJobs(char *filename, unsigned int *numJobs, char *envp[], int verbose) {
  unsigned int maxLineLength = MAX_ALIAS_LEN+PATH_MAX+50;
  char line[maxLineLength];
  unsigned int numLines;
  unsigned int id = 0;
  char alias[MAX_ALIAS_LEN];
  unsigned int startAfterMS;
  unsigned int maxDurationMS;
  char command[PATH_MAX];
  bgjob *b;
  char *regexString = "([^;]+);([^;]+);([^;]+);([^;]+)"; // startAfterMS
//char *regexString = "([^;]+);([^;]+);([^;]+)";
  regex_t regexCompiled;
//size_t maxGroups = 5; // startAfterMS
  size_t maxGroups = 5;
  char  *end;
  float  num;
  unsigned int g = 0;
  char sourceCopy[MAX_ALIAS_LEN+PATH_MAX+50 + 1];
  regmatch_t groupArray[maxGroups];
  char MSGBUFF[BUFSIZE];
  bgjob* jobs;
  char scanfFormat[20];

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

  sprintf(scanfFormat, "%%%ds", maxLineLength - 1);
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
  pid_t pid;
  char MSGBUFF[BUFSIZE];
  bgjob *job = (bgjob *) arg;

  if(job->verbose) {
    sprintf(MSGBUFF, "Thread for [%s]: forking from pid %d and thread id %lu to exec the job", job->alias, getpid(), pthread_self());
    tPrint(MSGBUFF);
  }

  // we must flush or the buffered output will be printed twice
  fflush(stdout);
  fflush(stderr);
  pid = fork();

  if (pid < 0) {
    /* error */
    fprintf(stderr, "Can't fork\n");
    exit(1);
  }
  else if (pid == 0) {

    /* child */
    if(job->verbose) {
      sprintf(MSGBUFF, "Thread for [%s]: child process for [%s] has pid [%u]", job->alias, job->command, getpid());
      tPrint(MSGBUFF);
    }
    if(job->startAfterMS > 0) {
      if(job->verbose) {
        sprintf(MSGBUFF, "Thread for [%s]: child process sleeps %dms", job->alias, job->startAfterMS);
        tPrint(MSGBUFF);
      }
      usleep(1000 * job->startAfterMS);
      if(job->verbose) {
        sprintf(MSGBUFF, "Thread for [%s]: child process awakes", job->alias);
        tPrint(MSGBUFF);
      }
    }

    if(job->verbose) {
      sprintf(MSGBUFF, "Thread for [%s]: child process executing [%s]", job->alias, job->command);
      tPrint(MSGBUFF);
    }

    // we must flush before duplicating file descriptors
    // or the pending buffered writes will be written to the new output files
    fflush(stdout);
    fflush(stderr);

    char childFileOut[PATH_MAX];
    char childFileErr[PATH_MAX];
    sprintf(childFileOut, "/tmp/bgrunner.job.%s.stdout", job->alias);
    sprintf(childFileErr, "/tmp/bgrunner.job.%s.stderr", job->alias);

    int outFd = open(childFileOut, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    int errFd = open(childFileErr, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if(outFd < 0 || errFd < 0) {
      fprintf(stderr, "Error opening stdout or stderr files on the child process of [%s]\n", job->alias);
      exit(1);
    }

    if(dup2(outFd, 1) < 0 || dup2(errFd, 2) < 0) {
      fprintf(stderr, "Error duplicating file descriptors on the child process of [%s]\n", job->alias);
      exit(1);
    }
    if(close(outFd) < 0 || close(errFd) < 0) {
      fprintf(stderr, "Error closing the old file descriptors after duplicating them on the child process of [%s]\n", job->alias);
      exit(1);
    }

    execl(job->command, job->command, (char *) NULL);

    /* exec() just returns on error */
    fprintf(stderr, "Error calling execl from the child process of [%s]\n", job->alias);
    exit(1);
  }
  else {
    /* parent */
    struct timeval now;
    gettimeofday(&now, NULL);

    job->pid         = pid;
    job->state       = STARTED;
    job->startupTime = now;

    if(job->verbose) {
      sprintf(MSGBUFF, "Thread for [%s]: parent after exec", job->alias);
      tPrint(MSGBUFF);
    }
  }
}


void launchJobs(char *filename, char *envp[], int verbose) {
  unsigned int numJobs;
  bgjob* jobs;
  pthread_t *threads;
  char MSGBUFF[BUFSIZE];

  jobs = loadJobs(filename, &numJobs, envp, verbose);
  threads = (pthread_t *) malloc(numJobs * sizeof(pthread_t));

  if(verbose) {
    printf("%u jobs described on %s:\n", numJobs, filename);
    for(int i = 0; i < numJobs; i++) {
      printf("* "); printJobShort(jobs+i);
    }
  }

  for(int i = 0; i < numJobs; i++) {

/*
    // using threads
    if(verbose > 1) {
      sprintf(MSGBUFF, "Creating the launcher thread for the job [%s]", jobs[i].alias);
      tPrint(MSGBUFF);
    }
    if(pthread_create(threads+i, NULL, execStartupRoutine, (void *) &(jobs[i]))) {
      fprintf(stderr, "Can't create the thread of [%s]\n", jobs[i].alias);
      exit(1);
    }
    // Let's create the childs one by one
    if(pthread_join(threads[i], NULL)) {
      fprintf(stderr, "Can't join to the thread of [%s]\n", jobs[i].alias);
      exit(1);
    }
    if(verbose > 1) {
      sprintf(MSGBUFF, "The launcher thread for the job [%s] has ended", jobs[i].alias);
      tPrint(MSGBUFF);
    }
*/

    if(verbose > 1) {
      sprintf(MSGBUFF, "Let's launch the job [%s] from pid %u", jobs[i].alias, getpid());
      tPrint(MSGBUFF);
    }
    execStartupRoutine(&(jobs[i]));
    if(verbose > 1) {
      sprintf(MSGBUFF, "The job [%s] has been launched from pid %u", jobs[i].alias, getpid());
      tPrint(MSGBUFF);
    }

  }

  waitForJobs(jobs, numJobs, verbose);
  free(threads);
  free(jobs);
}

