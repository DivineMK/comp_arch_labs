#ifndef _PARAMS_H_
#define _PARAMS_H_

#include "common.h"

typedef struct Params {
  unsigned int n_pixels;
  int n_warmup;
  int n_reps;
} Params;

static void usage() {
  fprintf(stderr, "\nUsage:  ./program [options]"
                  "\n"
                  "\nGeneral options:"
                  "\n    -h        help"
                  "\n    -w <W>    # of untimed warmup iterations (default=0)"
                  "\n    -e <E>    # of timed repetition iterations (default=1)"
                  "\n"
                  "\nWorkload-specific options:"
                  "\n    -p <N>    # of pixels (default=131072 pixels)"
                  "\n");
}

struct Params input_params(int argc, char **argv) {
  struct Params p;
  p.n_pixels = 131072;
  p.n_reps = 1;

  int opt;
  while ((opt = getopt(argc, argv, "hp:a:w:e:")) >= 0) {
    switch (opt) {
    case 'h':
      usage();
      exit(0);
      break;
    case 'p':
      p.n_pixels = atoi(optarg);
      break;
    case 'w':
      p.n_warmup = atoi(optarg);
      break;
    case 'e':
      p.n_reps = atoi(optarg);
      break;
    case ':':
      printf("option needs a value\r\n");
      break;
    case '?':
      printf("unknown option : %c\r\n", optopt);
      break;
    default:
      fprintf(stderr, "\nUnrecognized option!\n");
      usage();
      exit(0);
    }
  }
  assert(NR_DPUS > 0 && "Invalid # of dpus!");

  return p;
}
#endif
