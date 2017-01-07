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
#include <limits.h>       // PATH_MAX

#define MAX_JOBS 1024
#define MAX_ALIAS_LEN   50   // Max length of the alias

enum bgjstate {UNSTARTED, STARTED, KILLED, FINISHED}; 

/** Background job structure */
typedef struct {
  unsigned int   id;
  char           alias[MAX_ALIAS_LEN];
  unsigned int   startAfterMS;
  unsigned int   maxDurationMS;
  char           command[PATH_MAX];
  pid_t          pid;
  enum bgjstate  state;
  struct timeval startupTime;
  char         **envp;
  int            verbose;
} bgjob;

/* Funcs */

void printJob(bgjob *);

unsigned int countLines(char *);

bgjob *loadJobs(char *, unsigned int*, char *envp[], int);

void launchJobs(char *, char *envp[], int);

void waitForJobs(bgjob *, unsigned int, int);

#endif // BGRUNNER_H
