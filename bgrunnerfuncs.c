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

#define SLEEP_TIME 10000  // microseconds


/*
 * Difference between 2 points in time.
 *
 * @arg startup time
 * @arg startup time
 * @return difference in miliseconds
 */
double timeval_diff(struct timeval *a, struct timeval *b) {
// printf("Difference = %f\n", 1000 * ((double)(a->tv_sec + (double)a->tv_usec/1000000) - (double)(b->tv_sec + (double)b->tv_usec/1000000)));
  return 1000 * ((double)(a->tv_sec + (double)a->tv_usec/1000000) - (double)(b->tv_sec + (double)b->tv_usec/1000000));
}


void waitForJobs(bgjob *jobs, unsigned int numJobs, int verbose) {
  int finishedJobs;
  pid_t w;
  int status;
  unsigned int sleepTime = SLEEP_TIME;
  unsigned int z = 0;

  if(verbose) printf("Let's wait for the jobs with a sleepTime of %u us:\n", sleepTime);
  for(;;) {
    finishedJobs = 0;
    for(int i = 0; i < numJobs; i++) {
      if(jobs[i].state == STARTED) {
        w = waitpid(jobs[i].pid, &status, WNOHANG);
        if(w == jobs[i].pid) {
          jobs[i].state = FINISHED;
          finishedJobs++;
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
          struct timeval now;
          gettimeofday(&now, NULL);
// if(verbose) printf("Job [%s] has maxDurationMS = %u ms\n", jobs[i].alias, jobs[i].maxDurationMS);
          if(timeval_diff(&now, &(jobs[i].startupTime)) > jobs[i].maxDurationMS + jobs[i].startAfterMS) {
            if(verbose) printf("Job [%s] has been running more than %u ms. Let's kill it.\n", jobs[i].alias, jobs[i].maxDurationMS);
            kill(jobs[i].pid, SIGKILL);
          }
        }
      }
      else if(jobs[i].state == FINISHED) {
        finishedJobs++;
      }
    }
    if(finishedJobs == numJobs) {
      if(verbose) printf("All jobs finished\n");
      return;
    }

    if(verbose > 1)
      if(z * sleepTime >= 1000000) {
        z = 0;
        printf("%d finished jobs\n", finishedJobs);
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


unsigned int loadJobs(char *filename, bgjob** bgjdptr, int verbose) {
  char line[MAX_ALIAS_LEN+MAX_COMMAND_LEN+50];
  unsigned int numLines;
  unsigned int id = 0;
  char alias[MAX_ALIAS_LEN];
  unsigned int startAfterMS;
  unsigned int maxDurationMS;
  char command[MAX_COMMAND_LEN];
  bgjob *b;
  char *regexString = "([^;]+);([^;]+);([^;]+);([^;]+)";
  regex_t regexCompiled;
  size_t maxGroups = 5;
  char  *end;
  float  num;
  unsigned int g = 0;
  char sourceCopy[MAX_ALIAS_LEN+MAX_COMMAND_LEN+50 + 1];
  regmatch_t groupArray[maxGroups];

  if(verbose > 1)
    printf("Using regexp [%s] when parsing the job descriptor [%s]\n", regexString, filename);

  // example about C regexp: http://stackoverflow.com/a/11864144/939015
  // build regex
  if (regcomp(&regexCompiled, regexString, REG_EXTENDED)) {
    printf("Can't compile regular expression [%s]\n", regexString);
    exit(1);
  }

  // read length of file and alloc bgjob array
  numLines = countLines(filename);
  if(verbose > 1) printf("%u lines on job descriptor %s\n", numLines, filename);
  *bgjdptr = (bgjob*) malloc(numLines * sizeof(bgjob));

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
      b = *bgjdptr + id;
      b->id = id;
      strcpy(b->alias, alias);
      b->startAfterMS  = startAfterMS;
      b->maxDurationMS = maxDurationMS;
      strcpy(b->command, command);
      b->state = UNSTARTED;
      id++;
    }
  }
  
  fclose(myFile);

  // free allocated but unset memory (invalid or empty lines)
  return id;
}


void *execStartupRoutine (void *arg) {
  exec_args *args = (exec_args *) arg;

  if(args->verbose > 1) {
    printf("  thread for job [%s]: forking from pid %d to exec the job\n", args->job->alias, getpid());
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
    if(args->job->startAfterMS > 0) {
      if(args->verbose > 1) printf("  thread for job [%s]: child process sleeps %dms\n", args->job->alias, args->job->startAfterMS);
      usleep(1000 * args->job->startAfterMS);
      if(args->verbose > 1) printf("  thread for job [%s]: child process awakes\n", args->job->alias);
    }

    if(args->verbose) printf("  thread for job [%s]: child process executing [%s]\n", args->job->alias, args->args[0]);
    execve(args->args[0], args->args, args->envp);

    /* execve() just returns on error */
    fprintf(stderr, "Error calling execve from the child process\n");
    exit(1);
  }
  else {
    /* parent */
    struct timeval now;
    gettimeofday(&now, NULL);

    args->job->pid         = pid;
    args->job->state       = STARTED;
    args->job->startupTime = now;

//  if(args->verbose > 1) printf("  thread for job [%s]: parent after exec\n", args->job->alias);
  }
}


void launchJobs(char *filename, char *envp[], int verbose) {
  unsigned int numJobs;
  bgjob* jobs;

  pthread_t threads[numJobs];
  char *args[numJobs][2];
  exec_args execArgs[numJobs];

  numJobs = loadJobs(filename, &jobs, verbose);

  if(verbose) {
    printf("%u jobs described on %s:\n", numJobs, filename);
    for(int i = 0; i < numJobs; i++) {
      printf("* "); printJobShort(jobs+i);
    }
  }

  for(int i = 0; i < numJobs; i++) {

    args[i][0] = jobs[i].command;
    args[i][1] = '\0';

    execArgs[i].job     = jobs+i;
    execArgs[i].args    = args[i];
    execArgs[i].envp    = envp;
    execArgs[i].verbose = verbose;

    if(verbose > 1) printf("Creating the launcher thread for the job [%s]\n", (*((bgjob *)(jobs+i))).alias);

    if(pthread_create(threads+i, NULL, execStartupRoutine, (void *) &(execArgs[i])) ) {
      fprintf(stderr, "Can't create the thread\n");
      exit(1);
    }
  }

  waitForJobs(jobs, numJobs, verbose);
  free(jobs);
}

