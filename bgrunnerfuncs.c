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

// /* global lock */
// pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


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

void printJob(bgjob *b) {
  printf("BackGround job: id=[%u], alias=[%s], startAfterSeconds=[%u], maxDurationSeconds=[%u], command=[%s], pid=[%d], state=[%d]\n", b->id, b->alias, b->startAfterSeconds, b->maxDurationSeconds, b->command, b->pid, b->state);
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
//    if(verbose) printf("regex read      :        alias=%s, startAfterSeconds=%u, maxDurationSeconds=%u, command=%s\n", alias, startAfterSeconds, maxDurationSeconds, command);
      b = *bgjdptr + id;
      b->id = id;
      strcpy(b->alias, alias);
      b->startAfterSeconds  = startAfterSeconds;
      b->maxDurationSeconds = maxDurationSeconds;
      strcpy(b->command, command);
      b->state = UNSTARTED;
//    if(verbose) printf("struct assigned : id=%u, alias=%s, startAfterSeconds=%u, maxDurationSeconds=%u, command=%s\n", b->id, b->alias, b->startAfterSeconds, b->maxDurationSeconds, b->command);
      id++;
    }
  }
  
  fclose(myFile);

  // free allocated but unset memory (invalid or empty lines)
  return id;
}


void *execStartupRoutine (void *arg) {
  exec_args *args = (exec_args *) arg;

//  // Let's lock. Just one exec at a time
//  // Doesn't matter if the child has also the mutex locked, it's just to execve
//  pthread_mutex_lock(&mutex);

  if(args->verbose > 1) { printf("thread: forking from pid %d to exec this job: ", getpid()); printJob(args->job); }
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
      if(args->verbose > 1) printf("thread: child sleeping %d seconds before execve [%s]\n", args->job->startAfterSeconds, args->args[0]);
      sleep(args->job->startAfterSeconds);
      if(args->verbose > 1) printf("thread: child awaken after sleeping %d seconds before execve [%s]\n", args->job->startAfterSeconds, args->args[0]);
    }
    if(args->verbose) printf("thread: child going to execve [%s]\n", args->args[0]);
    execve(args->args[0], args->args, args->envp);

    /* execve() just returns on error */
    fprintf(stderr, "Error calling execve from the thread\n");
    exit(1);
  }
  else {
    /* parent */
    if(args->verbose > 1) printf("thread: parent after execve %s\n", args->args[0]);
    args->job->pid   = pid;
    args->job->state = STARTED;

//    // Let's unlock
//    pthread_mutex_unlock(&mutex);
  }
}


// void launchJob(pthread_t *thread, exec_args *execArgs) {
// printf("launchJob: before pthread_create. Executable= %s\n", execArgs->args[0]);
//   if(pthread_create(thread, NULL, execStartupRoutine, (void *) execArgs) ) {
//     fprintf(stderr, "Can't create the thread\n");
//     exit(1);
//   }
// }


void launchJobs(char *filename, char *envp[], int verbose) {
  unsigned int numJobs;
  bgjob* jobs;

  pthread_t threads[numJobs];
  char *args[numJobs][2];
  exec_args execArgs[numJobs];

  numJobs = loadJobs(filename, &jobs, verbose);
  if(verbose) printf("launchJobs: Launching %u jobs described on %s\n", numJobs, filename);

  for(int i = 0; i < numJobs; i++) {

    args[i][0] = jobs[i].command;
    args[i][1] = '\0';

    execArgs[i].job     = jobs+i;
    execArgs[i].args    = args[i];
    execArgs[i].envp    = envp;
    execArgs[i].verbose = verbose;

    if(verbose > 1) printf("launchJobs: %d: Creating a thread for the executable: %s\n", i, (*((bgjob *)(jobs+i))).command);
//  printf("launchJobs: %d: before launchJob. Executable= %s\n", i, execArgs[i].args[0]);
//  launchJob(threads+i, &(execArgs[i]));

    if(pthread_create(threads+i, NULL, execStartupRoutine, (void *) &(execArgs[i])) ) {
      fprintf(stderr, "Can't create the thread\n");
      exit(1);
    }

    if(verbose > 1) { printf("launchJobs: Created job: "); printJob(jobs+i); }
  }

  waitForJobs(jobs, numJobs, verbose);
  free(jobs);
}


void waitForJobs(bgjob *jobs, unsigned int numJobs, int verbose) {
  int finishedJobs;
  pid_t w;
  int status;
  unsigned int sleepTime = 10000; // 10ms
  unsigned int z = 0;

  if(verbose) printf("Let's wait for jobs with sleepTime = %u us:\n", sleepTime);
  for(;;) {
    finishedJobs = 0;
    for(int i = 0; i < numJobs; i++) {

//  $r = waitpid($pid, WNOHANG | WUNTRACED);
//  print "pid=$pid, r=$r\n";
//  if($r == 0) {
//    print "Process $pid still running\n";
//  }
//  elsif($r == $pid) {
//    print "Process $pid just finished\n";
//  }
//  elsif($r == -1) {
//    print "Process $pid not running\n";
//  }

//    if(verbose > 1) printf("Checking the process #%d from the process %d:", i, getpid());
//    if(verbose > 1) printJob(&jobs[i]);

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
        printf("%d finishedJobs\n", finishedJobs);
      }
    usleep(sleepTime);
    z++;
  }
}

