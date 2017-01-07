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
#include <limits.h>       // PATH_MAX, ...

#include "bgrunner.h"


void usage() {
  printf("Background runner\n");
  printf("Usage:\n");
  printf("bgrunner (-v) (-d) -f <jobsdescriptor>\n");
  exit(1);
}


void getOpts(int argc, char **argv, int *verbose, char *filename) {
  int c;
  extern char *optarg;
  extern int optind, opterr, optopt;
  opterr = 0;
  short v = 0, d = 0;

  if(argc == 2 && strcmp(argv[1], "-h") == 0) {
    usage();
  }
  if(argc < 3) {
    fprintf(stderr, "Missing parameters\n");
    usage();
  }

  char scanfFormat[20];
  sprintf(scanfFormat, "%%%ds", PATH_MAX - 1);
  while ((c = getopt (argc, argv, "vdf:")) != -1) {
    switch (c) {
      case 'h':
        usage();
        break;
      case 'v':
        v = 1;
        break;
      case 'd':
        d = 1;
        break;
      case 'f':
        if(sscanf(optarg, scanfFormat, filename) != 1) {
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

    if(d)
      *verbose = 2;
    else
      *verbose = v;
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
int main (int argc, char *argv[], char *envp[]) {
  /* verbose will be:
   * 0 == non-verbose
   * 1 == verbose      (-v)
   * 2 == more verbose (-d)
   */
  int  verbose = 0;
  char filename[PATH_MAX];

  getOpts(argc, argv, &verbose, filename);
  launchJobs(filename, envp, verbose);

  exit(0);
}

