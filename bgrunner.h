/*
 * Background jobs runner header file
 * 
 * @since 20161211
 * @author zoquero@gmail.com
 */

#ifndef BGRUNNER_H
#define BGRUNNER_H

#define MAX_JOBS 1024
#define MAX_ALIAS_LEN   50   // Max length of the alias
#define MAX_COMMAND_LEN 1024 // Max length of the command


enum bgjstate {UNSTARTED, STARTED, KILLED, FINISHED}; 

/** Background job structure */
typedef struct {
  unsigned int  id;
  char          alias[MAX_ALIAS_LEN];
  unsigned int  startAfterSeconds;
  unsigned int  maxDurationSeconds;
  char          command[MAX_COMMAND_LEN];
  enum bgjstate state;
} bgjob;


// Funcs

void job2stdout(bgjob *);

unsigned int countLines(char *);

unsigned int loadJobs(char *, bgjob**, int);

void checkJob(bgjob*);

void launchJob(bgjob*);

void launchJobs(char *, int);

#endif // BGRUNNER_H

