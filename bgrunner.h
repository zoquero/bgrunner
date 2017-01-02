/*
 * Background jobs runner header file
 * 
 * @since 20161211
 * @author zoquero@gmail.com
 */

#ifndef BGRUNNER_H
#define BGRUNNER_H

#include <sys/types.h>    // pid_t
#include <unistd.h>       // pid_t

#define MAX_JOBS 1024
#define MAX_ALIAS_LEN   50   // Max length of the alias
#define MAX_COMMAND_LEN 1024 // Max length of the command

enum bgjstate {UNSTARTED, STARTED, KILLED, FINISHED}; 

/** Background job structure */
typedef struct {
  unsigned int   id;
  char           alias[MAX_ALIAS_LEN];
  unsigned int   startAfterMS;
  unsigned int   maxDurationMS;
  char           command[MAX_COMMAND_LEN];
  pid_t          pid;
  enum bgjstate  state;
  struct timeval startupTime;
} bgjob;

typedef struct {
  bgjob *job;
  char  **args;
  char  **envp;
  int  verbose;
} exec_args;

/* Funcs */

void printJob(bgjob *);

unsigned int countLines(char *);

unsigned int loadJobs(char *, bgjob**, int);

void launchJobs(char *, char *envp[], int);

void waitForJobs(bgjob *, unsigned int, int);

#endif // BGRUNNER_H

