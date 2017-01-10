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
#define BUFSIZE  1024
#define MAX_ARGS  100
#define MAX_ALIAS_LEN       50      // Max length of the alias
#define STATE_PREFORK       0
#define STATE_FORKED        1
#define STATE_EXEC_ERROR    2
#define DEFAULT_FOLDER      "/tmp"
#define SLEEP_TIME_US       1000    // in microseconds
#define US_TO_SHOW_ON_DEBUG 1000000 // 1 second
#define RESULTS_BASENAME    "bgrunner.results.csv"

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
  char        ** envp;
  int            verbose;
} bgjob;

/* Funcs */

unsigned int countLines(char *);
bgjob *loadJobs(char *, unsigned int*, char *envp[], int);
void launchJobs(char *, char *, char *envp[], int);
void launchJob(void *, char *, char *);
void waitForJobs(bgjob *, char *, unsigned int, int);
int split(char *, char **, char *, int);
void tPrint (char *);
double timeval_diff(struct timeval *, struct timeval *);
void printJobShort(bgjob *);
void printJob(bgjob *);
void printJobFull(bgjob *);

#endif // BGRUNNER_H
