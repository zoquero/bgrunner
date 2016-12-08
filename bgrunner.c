/*
 * Background runner
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

/*******
void parseParams(char *params, enum btype thisType, int verbose, unsigned long *times, unsigned long *sizeInBytes, unsigned int *nThreads, char *folderName, char *targetFileName, char *url, char *httpRefFileBasename, unsigned long *timeoutInMS, char *dest, double warn, double crit) {
  if(thisType == CPU) {
    if(strlen(params) > 19) {
      fprintf(stderr, "Params must be in \"num,num\" format\n");
      usage();
    }
    if(sscanf(params, "%lu,%u", times, nThreads) != 2) {
      *nThreads = 1;
      if(sscanf(params, "%lu", times) != 1) {
        fprintf(stderr, "Params must be in \"num,num\" format\n");
        usage();
      }
    }
    if(verbose)
      printf("type=cpu, times=%lu, nThreads=%u, warnLevel=%f, critLevel=%f, verbose=%d\n", *times, *nThreads, warn, crit, verbose);
  }
  else if(thisType == MEM) {
    if(sscanf(params, "%lu,%lu", times, sizeInBytes) != 2) {
      fprintf(stderr, "Params must be in \"num,num\" format\n");
      usage();
    }
    if(verbose)
      printf("type=mem, times=%lu, sizeInBytes=%lu, warnLevel=%f, critLevel=%f, verbose=%d\n", *times, *sizeInBytes, warn, crit, verbose);
  }
  else if(thisType == DISK_W) {
    if(sscanf(params, "%lu,%lu,%u,%s", times, sizeInBytes, nThreads, folderName) != 4) {
      *nThreads = 1;
      if(sscanf(params, "%lu,%lu,%s", times, sizeInBytes, folderName) != 3) {
        fprintf(stderr, "Params must be in \"num,num,(num,)path\" format\n");
        usage();
      }
    }
    if(verbose)
      printf("type=disk_w, times=%lu, sizeInBytes=%lu, nThreads=%d, folderName=%s, warnLevel=%f, critLevel=%f, verbose=%d\n", *times, *sizeInBytes, *nThreads, folderName, warn, crit, verbose);
  }
  else if(thisType == DISK_R_SEQ) {
    *nThreads = 1;
    if(sscanf(params, "%lu,%lu,%s", times, sizeInBytes, targetFileName) != 3) {
      fprintf(stderr, "Params must be in \"num,num,path\" format\n");
      usage();
    }
    if(verbose) {
      printf("type=disk_r_seq, times=%lu, sizeInBytes=%lu, targetFileName=%s verbose=%d\n", *times, *sizeInBytes, targetFileName, verbose);
    }
  }
  else if(thisType == DISK_R_RAN) {
    if(sscanf(params, "%lu,%lu,%u,%s", times, sizeInBytes, nThreads, targetFileName) != 4) {
      *nThreads = 1;
      if(sscanf(params, "%lu,%lu,%s", times, sizeInBytes, targetFileName) != 3) {
        fprintf(stderr, "Params must be in \"num,num,(num),path\" format\n");
        usage();
      }
    }
    if(verbose) {
      printf("type=disk_r_ran, times=%lu, sizeInBytes=%lu, nThreads=%u, targetFileName=%s verbose=%d\n", *times, *sizeInBytes, *nThreads, targetFileName, verbose);
    }
  }
  else if(thisType == HTTP_GET) {
    if(sscanf(params, "%[^,],%s", httpRefFileBasename, url) != 2) {
      fprintf(stderr, "Params must be in \"refName,url\" format\n");
      usage();
    }
    if(verbose)
      printf("type=http_get, httpRefFileBasename=%s, url=%s, verbose=%d\n", httpRefFileBasename, url, verbose);
  }
// ifdef OPING_ENABLED
  else if(thisType == PING) {
    if(sscanf(params, "%lu,%lu,%s", times, sizeInBytes, dest) != 3) {
      fprintf(stderr, "Params must be in \"times,sizeInBytes,dest\" format\n");
      usage();
    }
    if(verbose)
      printf("type=ping, sizeInBytes=%lu, times=%lu, dest=%s, verbose=%d\n", *sizeInBytes, *times, dest, verbose);
  }
// endif // OPING_ENABLED
  else {
    fprintf(stderr, "Unknown o missing type\n");
    usage();
  }
}
**********/


void getOpts(int argc, char **argv, int *verbose, char *fileName) {
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
        if(sscanf(optarg, "%s", fileName) != 1) {
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

void launchJobs(char *fileName, int verbose) {
  printf("Launching jobs described on %s, verbose=%d\n", fileName, verbose);
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
  char fileName[PATH_MAX];

  getOpts(argc, argv, &verbose, fileName);
  launchJobs(fileName, verbose);
  exit(0);
}
