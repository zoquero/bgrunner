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
#include <sys/time.h>     // gettimeofday
#include <time.h>         // strftime , gmtime_r, time, struct tm
#include <signal.h>       // kill
#include <fcntl.h>        // open
#include <sys/mman.h>     // mmap

#include "bgrunner.h"


/* shmChildStates is for IPC, for child to tell the parent if could do exec.
 * It has nothing to do with bgjob.state.
 */
static char *shmChildStates;

/**
  * Split a string into an array
  * @param pointer to the char array to be splitted
  * @param pointer to the pointer to the first element
  *          of the array that should contain the results.
  *          It must be allocated by the caller,
  *          like for example: char *array[max_components];
  * @param pointer to the delimitating character
  * @param maximum number of components
  * @return number of components or -1 if reaches the max number of components
  *
  */
int split(char *string, char **splitted_string, char *delim, int max_components) {
  int i=0;

  splitted_string[i] = strtok(string, delim);
  while(splitted_string[i]!=NULL) {
    if(i >= max_components)
      return -1;
    splitted_string[++i] = strtok(NULL, delim);
  }
  return i;
}


/**
  * Print to stdout with UTC timestamp
  * @arg s string to write
  */
void tPrint (char *s) {
  
  time_t     rawtime;
  char       buffer[80];
  struct tm  result;
  time(&rawtime);
  if(gmtime_r(&rawtime, &result) != NULL) {
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


void waitForJobs(bgjob *jobs, char *outputFolder, unsigned int numJobs, int verbose) {
  int finishedJobs;
  pid_t w;
  int status;
  unsigned int sleepTime = SLEEP_TIME_US;
  unsigned int z = 0;
  char MSGBUFF[BUFSIZE];
  char wExitStatus;
  char outputFilename[PATH_MAX];
  short killed[numJobs];

  sprintf(outputFilename, "%s/%s", outputFolder, RESULTS_BASENAME);

  if(verbose > 1) {
    sprintf(MSGBUFF, "Let's open output file with results %s", outputFilename);
    tPrint(MSGBUFF);
    fflush(stdout);
  }
  FILE *resultsFile;
  resultsFile=fopen(outputFilename , "w");
  if(resultsFile == NULL) {
    sprintf(MSGBUFF, "ERROR: Can't open the results file\n");
    tPrint(MSGBUFF);
    fflush(stdout);
  }
  else {
    fprintf(resultsFile, "#job_alias;job_command;wait_ret_code;killedByTimeout(0==false,1==true);execResult(1==ok,2==error);durationMS(sleepTime=%dus)\n",sleepTime);
  }

  if(verbose) {
    sprintf(MSGBUFF, "Let's wait for the jobs with a sleepTime of [%u] us", sleepTime);
    tPrint(MSGBUFF);
    fflush(stdout);
  }
  for(;;) {
    finishedJobs = 0;
    for(int i = 0; i < numJobs; i++) {
      if(jobs[i].state == STARTED) {
        w = waitpid(jobs[i].pid, &status, WNOHANG);
        if(w == jobs[i].pid) {
          jobs[i].state = FINISHED;
          finishedJobs++;
          if(WIFEXITED(status)) {
            wExitStatus = WEXITSTATUS(status);
            if(verbose) {
              if(shmChildStates[i] == STATE_EXEC_ERROR) {
                sprintf(MSGBUFF,
                  "Job [%s]: It couldn't be executed (execve failed)", jobs[i].alias);
                tPrint(MSGBUFF);
                fflush(stdout);
              }
              else {
                sprintf(MSGBUFF, "Job [%s]: It finished with return code [%d]", jobs[i].alias, (int) wExitStatus);
                tPrint(MSGBUFF);
                fflush(stdout);
              }
            }
          }
          else {
            if(killed[i] == 1) {
              sprintf(MSGBUFF, "Job [%s]: It has been killed by timeout", jobs[i].alias);
            }
            else {
              sprintf(MSGBUFF, "Job [%s]: It haven't finished normally (WIFEXITED returns false), maybe was killed by someone else", jobs[i].alias);
            }
            tPrint(MSGBUFF);
            fflush(stdout);
          }

          struct timeval now;
          double durationMS=timeval_diff(&now, &(jobs[i].startupTime)) - jobs[i].startAfterMS;
          if(resultsFile != NULL)
            fprintf(resultsFile, "%s;%s;%d;%d;%d;%f\n", jobs[i].alias, jobs[i].command, (int) wExitStatus, killed[i], (int) shmChildStates[i], durationMS);
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
              sprintf(MSGBUFF, "Job [%s]: has been running more than [%u] ms. Let's kill it", jobs[i].alias, jobs[i].maxDurationMS);
              tPrint(MSGBUFF);
              fflush(stdout);
              kill(jobs[i].pid, SIGKILL);
              killed[i] = 1;
            }
          }
        }
      }
      else if(jobs[i].state == FINISHED) {
        finishedJobs++;
      }
    }
    if(finishedJobs == numJobs) {
      if(verbose) {
        sprintf(MSGBUFF, "All jobs finished. Results saved at [%s]", outputFilename);
        tPrint(MSGBUFF);
        fflush(stdout);
      }
      return;
    }

    if(verbose > 1)
      if(z * sleepTime >= US_TO_SHOW_ON_DEBUG) {
        z = 0;
        sprintf(MSGBUFF, "%d finished jobs", finishedJobs);
        tPrint(MSGBUFF);
        fflush(stdout);
      }
    usleep(sleepTime);
    z++;
  }

  if(fclose(resultsFile) != 0) {
    sprintf(MSGBUFF, "ERROR: Can't close the CSV output file with results");
    tPrint(MSGBUFF);
    fflush(stdout);
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
  printf("BackGround job with alias=[%s] that after [%u] ms will run [%s] for up to [%u] ms\n", b->alias, b->startAfterMS, b->command, b->maxDurationMS);
}


void printJob(bgjob *b) {
  printf("BackGround job with alias=[%s] that after [%u] ms will run [%s] for up to [%u] ms, with pid [%d] and state [%d]\n", b->alias, b->startAfterMS, b->command, b->maxDurationMS, b->pid, b->state);
}

void printJobFull(bgjob *b) {
  printf("BackGround job with id=[%d], alias=[%s], startAfterMS=[%u] ms, maxDurationMS=[%u] ms, command=[%s] ms, pid=[%u], state=[%d], startupTime=[PENDING], envp=[PENDING], verbose=[%d]\n", b->id, b->alias, b->startAfterMS, b->maxDurationMS, b->command, b->pid, b->state, /* b->startupTime, b->envp, */ b->verbose);
}



bgjob *loadJobs(char *filename, unsigned int *numJobs, char *envp[], int verbose) {
  // aprox 10 characters per param , 50 for separators and miliseconds
  unsigned int maxLineLength = MAX_ALIAS_LEN+PATH_MAX+10*MAX_ARGS+50;
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
    sprintf(MSGBUFF, "Using regexp [%s] and [%d] max line length when parsing the job descriptor [%s]", regexString, maxLineLength, filename);
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

  sprintf(scanfFormat, "%%%d[^\n]\n", maxLineLength - 1);
//printf("scanfFormat=[%s]\n", scanfFormat);
  while(fscanf(myFile, scanfFormat, line) == 1) {
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


void launchJob(void *arg, char *shmChildState, char *outputFolder) {
  pid_t pid;
  char MSGBUFF[BUFSIZE];
  bgjob *job = (bgjob *) arg;
  char childFileOut[PATH_MAX];
  char childFileErr[PATH_MAX];

  *shmChildState = STATE_PREFORK;

  if(job->verbose > 1) {
    sprintf(MSGBUFF,
      "Job [%s]: forking from pid [%d] to exec the job", job->alias, getpid());
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
    *shmChildState = STATE_FORKED;
    if(job->verbose > 1) {
      sprintf(MSGBUFF,
          "Job [%s]: child process for [%s] has pid [%u]",
          job->alias, job->command, getpid());
      tPrint(MSGBUFF);
    }
    if(job->startAfterMS > 0) {
      if(job->verbose > 1) {
        sprintf(MSGBUFF,
          "Job [%s]: child process sleeps [%d] ms before running the command",
          job->alias, job->startAfterMS);
        tPrint(MSGBUFF);
      }
      usleep(1000 * job->startAfterMS);
      if(job->verbose > 1) {
        sprintf(MSGBUFF, "Job [%s]: child process awakes", job->alias);
        tPrint(MSGBUFF);
      }
    }

    if(job->verbose) {
      sprintf(MSGBUFF,
        "Job [%s]: child process goint to execute [%s]",
        job->alias, job->command);
      tPrint(MSGBUFF);
    }

    /*
     * We must flush before duplicating file descriptors
     * or the pending buffered writes will be written to the new output files
     */
    fflush(stdout);
    fflush(stderr);
    sprintf(childFileOut, "%s/bgrunner.%s.stdout", outputFolder, job->alias);
    sprintf(childFileErr, "%s/bgrunner.%s.stderr", outputFolder, job->alias);

    int outFd = open(childFileOut, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    int errFd = open(childFileErr, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if(outFd < 0 || errFd < 0) {
      fprintf(stderr,
        "Job [%s]: Error opening stdout or stderr files on the child process\n",
        job->alias);
      exit(1);
    }

    if(dup2(outFd, 1) < 0 || dup2(errFd, 2) < 0) {
      fprintf(stderr,
        "Job [%s]: Error duplicating file descriptors on the child process\n",
        job->alias);
      exit(1);
    }
    if(close(outFd) < 0 || close(errFd) < 0) {
      fprintf(stderr,
        "Job [%s]: Error closing the old file descriptors "
        "after duplicating them on the child process\n",
        job->alias);
      exit(1);
    }

    // Let's brake the command into executable and arguments:

    char *args[MAX_ARGS+2]; // +1 for executable , +1 for the ending zero
    int numArgs = split(job->command, args, " ", MAX_ARGS + 1); // +1 for exec
    args[numArgs] = '\0'; // zero ended array of char*
    if(numArgs == -1) {
      *shmChildState = STATE_EXEC_ERROR;
      fprintf(stderr,
        "Job [%s]: Error parsing arguments of the command [%s]. "
        "Max number of arguments = %d\n",
        job->alias, job->command, MAX_ARGS);
      exit(1);
    }

    if(job->verbose > 1) {
      sprintf(MSGBUFF,
        "Job [%s]: Let's show the arguments that will be sent to the command:",
        job->alias);
      tPrint(MSGBUFF);
      for(int z=0; z<numArgs; z++) {
        sprintf(MSGBUFF,
          "Job [%s]: arg[%d]=[%s]", job->alias, z, args[z]);
        tPrint(MSGBUFF);
      }
    }

//  execl(job->command, job->command, (char *) NULL);
    execve(args[0], args, job->envp);

    /* exec() just returns on error */
    *shmChildState = STATE_EXEC_ERROR;
    fprintf(stderr,
      "Job [%s]: Error calling execve from the child, command [%s]\n",
      job->alias, job->command);
    exit(1);
  }
  else {
    /* parent */
    struct timeval now;
    gettimeofday(&now, NULL);

    job->pid         = pid;
    job->state       = STARTED;
    job->startupTime = now;

    if(job->verbose > 1) {
      sprintf(MSGBUFF, "Job [%s]: parent after exec", job->alias);
      tPrint(MSGBUFF);
    }
  }
}


void launchJobs(char *filename, char *outputFolder, char *envp[], int verbose) {
  unsigned int numJobs;
  bgjob* jobs;
  char MSGBUFF[BUFSIZE];

  jobs = loadJobs(filename, &numJobs, envp, verbose);

  shmChildStates = mmap(NULL, numJobs * sizeof(char), PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  if(verbose) {
    printf("%u jobs described on %s:\n", numJobs, filename);
    for(int i = 0; i < numJobs; i++) {
      printf("* "); printJobShort(jobs+i);
    }
    printf("Output files and reports will be created at [%s] folder,\n"
           "please, take a look at those files for troubleshooting.\n",
      outputFolder);
    printf("Let's begin:\n\n");
  }

  for(int i = 0; i < numJobs; i++) {

    if(verbose > 1) {
      sprintf(MSGBUFF, "Let's work with the job [%s] from pid [%u]", jobs[i].alias, getpid());
      tPrint(MSGBUFF);
    }
    launchJob(&(jobs[i]), &(shmChildStates[i]), outputFolder);
    if(verbose > 1) {
      sprintf(MSGBUFF, "The job [%s] has been launched from pid [%u]", jobs[i].alias, getpid());
      tPrint(MSGBUFF);
    }

  }

  waitForJobs(jobs, outputFolder, numJobs, verbose);
  munmap(shmChildStates, numJobs * sizeof(char));
  free(jobs);
}

