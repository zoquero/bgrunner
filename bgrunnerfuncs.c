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
#include <sys/types.h>    // pid_t
#include <unistd.h>       // pid_t
#include <sys/wait.h>     // waitpid
#include <pthread.h>      // pthread_create ...
#include <unistd.h>       // usleep


#include "bgrunner.h"


void waitForJobs(bgjob *jobs, unsigned int numJobs, int verbose) {
  int finishedJobs;
  pid_t w;
  int status;
  unsigned int sleepTime = 10000; // 10ms
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
          fprintf (stderr, "Bug: job #%u and pid %d has already finished\n", jobs[i].id, jobs[i].pid);
          exit(1);
        }
        else if(w != 0) {  // 0 is for already running child
          fprintf (stderr, "Unknown state for job #%u: %d\n", jobs[i].id, jobs[i].state);
          exit(1);
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
  printf("BackGround job with alias=[%s] that after %us will run [%s] for up to %us\n", b->alias, b->startAfterSeconds, b->command, b->maxDurationSeconds);
}


void printJob(bgjob *b) {
  printf("BackGround job with alias=[%s] that after %us will run [%s] for up to %us, with pid [%d] and state [%d]\n", b->alias, b->startAfterSeconds, b->command, b->maxDurationSeconds, b->pid, b->state);
}


unsigned int loadJobs(char *filename, bgjob** bgjdptr, int verbose) {
  char line[MAX_ALIAS_LEN+MAX_COMMAND_LEN+50];
  unsigned int numLines;
  unsigned int id = 0;
  char alias[MAX_ALIAS_LEN];
  unsigned int startAfterSeconds;
  unsigned int maxDurationSeconds;
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
      startAfterSeconds  = 0;
      maxDurationSeconds = 0;
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
          if(sscanf(sourceCopy + groupArray[g].rm_so, "%u", &startAfterSeconds) != 1) {
            fprintf(stderr, "Can't read startAfterSeconds in line [%s] on descriptor %s\n", line, filename);
            exit(1);
          }
          break;
        case 3:
          if(sscanf(sourceCopy + groupArray[g].rm_so, "%u", &maxDurationSeconds) != 1) {
            fprintf(stderr, "Can't read maxDurationSeconds in line [%s] on descriptor %s\n", line, filename);
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
      b->startAfterSeconds  = startAfterSeconds;
      b->maxDurationSeconds = maxDurationSeconds;
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
    if(args->job->startAfterSeconds > 0) {
      if(args->verbose > 1) printf("  thread for job [%s]: child process sleeps %ds\n", args->job->alias, args->job->startAfterSeconds);
      sleep(args->job->startAfterSeconds);
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
    if(args->verbose > 1) printf("  thread for job [%s]: parent after exec\n", args->job->alias);
    args->job->pid   = pid;
    args->job->state = STARTED;
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

