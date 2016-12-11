/*
 * Background jobs runner
 * 
 * Sources: https://github.com/zoquero/bgrunner/
 * 
 * @since 20161208
 * @author zoquero@gmail.com
 */

#include <stdio.h>        // printf
#include <stdlib.h>       // exit
#include <ctype.h>        // isprint
#include <getopt.h>       // getopt
#include <errno.h>        // errno
#include <stdint.h>       // intmax_t
#include <string.h>       // intmax_t
#include <limits.h>       // LONG_MAX, ...

#include "bgrunner.h"

void usage() {
  printf("Background runner\n");
  printf("Usage:\n");
  printf("bgrunner (-v) -f <jobsdescriptor>\n");
  exit(1);
}

unsigned long parseUL(char *str, char *valNameForErrors) {
  unsigned long r;
  char *endptr;
  r = strtol(str, &endptr, 10);
  if((errno == ERANGE && (r == LONG_MAX || r == LONG_MIN)) || (errno != 0 && r == 0)) {
    fprintf(stderr, "Error parsing \"%s\". Probably value too long.\n", valNameForErrors);
    usage();
  }
  if(endptr == str) {
    fprintf(stderr, "No digits found parsing \"%s\"\n", valNameForErrors);
    usage();
  }
  return r;
}


void getOpts(int argc, char **argv, int *verbose, char *filename) {
  int c;
  extern char *optarg;
  extern int optind, opterr, optopt;
  opterr = 0;

  if(argc == 2 && strcmp(argv[1], "-h") == 0) {
    usage();
  }
  if(argc < 3) {
    fprintf(stderr, "Missing parameters\n");
    usage();
  }

  while ((c = getopt (argc, argv, "vf:")) != -1) {
    switch (c) {
      case 'h':
        usage();
        break;
      case 'v':
        *verbose = 1;
        break;
      case 'f':
        if(sscanf(optarg, "%s", filename) != 1) {
          fprintf (stderr, "Option -%c requires an argument\n", c);
          usage();
        }
        break;
      case '?':
        if (optopt == 'p')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
        usage();
      default:
        fprintf (stderr,
          "Wrong command line arguments, probably a parameter without data\n");
        usage();
    }
  }
}

/**
  * Main.
  *
  * Some guidelines about developing nagios plugins:
  * https://nagios-plugins.org/doc/guidelines.html
  * http://blog.centreon.com/good-practices-how-to-develop-monitoring-plugin-nagios/
  *
  */
int main (int argc, char *argv[]) {
  int  verbose = 0;
  char filename[PATH_MAX];

  getOpts(argc, argv, &verbose, filename);
  launchJobs(filename, verbose);
  exit(0);
}
