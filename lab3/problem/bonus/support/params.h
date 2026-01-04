#ifndef _PARAMS_H_
#define _PARAMS_H_

#include "common.h"

typedef struct Params {
  unsigned int n_image;
  unsigned int image_dim;
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
                  "\n    -n <N>    # of images (default=128 images)"
                  "\n    -d <D>    dimension of image (default=32)"
                  "\n");
}

struct Params input_params(int argc, char **argv) {
  struct Params p;
  p.image_dim = 32;
  p.n_image = 128;
  p.n_warmup = 0;
  p.n_reps = 1;

  int opt;
  while ((opt = getopt(argc, argv, "hd:n:a:w:e:")) >= 0) {
    switch (opt) {
    case 'h':
      usage();
      exit(0);
      break;
    case 'd':
      p.image_dim = atoi(optarg);
      break;
    case 'n':
      p.n_image = atoi(optarg);
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
