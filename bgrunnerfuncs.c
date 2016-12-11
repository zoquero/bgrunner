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
#include <string.h>

#include "bgrunner.h"


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

void job2stdout(bgjob *b) {
  printf("BackGround job: id=%u, alias=%s, startAfterSeconds=%u, maxDurationSeconds=%u, command=%s\n", b->id, b->alias, b->startAfterSeconds, b->maxDurationSeconds, b->command);
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
  if(verbose) printf("%u lines on job descriptor %s\n", numLines, filename);
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
//    if(verbose) printf("struct assigned : id=%u, alias=%s, startAfterSeconds=%u, maxDurationSeconds=%u, command=%s\n", b->id, b->alias, b->startAfterSeconds, b->maxDurationSeconds, b->command);
      id++;
    }
  }
  
  fclose(myFile);

  // free allocated but unset memory (invalid or empty lines)
  return id;
}

void checkJob(bgjob* job) {
  printf("checkJob \n");
}

void launchJob(bgjob* job) {
  printf("Launching this job:\n\t");
  job2stdout(job);
}

void launchJobs(char *filename, int verbose) {
  unsigned int numJobs;
  bgjob* jobs;
  if(verbose) printf("Launching jobs described on %s, verbose=%d\n", filename, verbose);
  numJobs = loadJobs(filename, &jobs, verbose);
  if(verbose) printf("Loaded %u jobs\n", numJobs);
  for(int i = 0; i < numJobs; i++) {
    launchJob(jobs+i);
  }
  free(jobs);
}

