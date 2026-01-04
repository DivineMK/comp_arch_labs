/**
 * app.c
 * Host Application Source File
 *
 */
#include <assert.h>
#include <dpu.h>
#include <dpu_log.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../support/common.h"
#include "../support/params.h"
#include "../support/timer.h"

// Define the DPU Binary path as DPU_BINARY here
#ifndef DPU_BINARY
#define DPU_BINARY "./bin/dpu_code"
#endif
// Pointer declaration
static pixel_t *Y;
static pixel_t *Y_host;

// Create input arrays
static void read_input(pixel_t *A, unsigned int nr_elements) {
  srand(0);
  printf("nr_elements\t%u\n", nr_elements);
  for (unsigned int i = 0; i < nr_elements; i++) {
    A[i].r = (uint8_t)(rand());
    A[i].g = (uint8_t)(rand());
    A[i].b = (uint8_t)(rand());
  }
}

// Compute output in the host for verification purposes
static void grayscale_host(pixel_t *A, unsigned int nr_elements) {
  for (unsigned int i = 0; i < nr_elements; i++) {
    uint8_t grayscale;
#ifdef LIG
    grayscale =
        (min(min(A[i].r, A[i].g), A[i].b) + max(max(A[i].r, A[i].g), A[i].b)) >>
        1;
#elif AVG
    grayscale =
        (uint32_t)((A[i].r + A[i].g + A[i].b) * 342) >> 10; // 1/3=342>>10
#elif LUM
    grayscale = ((uint32_t)(307 * A[i].r) + (uint32_t)(604 * A[i].g) +
                 (uint32_t)(113 * A[i].b)) >>
                10;
#else
    printf("Invalid grayscale method\r\n");
    return;
#endif
    A[i].r = A[i].g = A[i].b = grayscale;
  }
}

// Main of the Host Application
int main(int argc, char **argv) {

  // Input parameters
  struct Params p = input_params(argc, argv);

  // Timer declaration
  Timer timer;
#if defined(CYCLES) || defined(INSTRUCTIONS)
  double cc = 0;
  double cc_min = 0;
#endif

  // Allocate DPUs
  struct dpu_set_t dpu_set, dpu;
  uint32_t nr_of_dpus, nr_of_ranks;
  DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &dpu_set));
  DPU_ASSERT(
      dpu_get_nr_dpus(dpu_set, &nr_of_dpus)); // Number of DPUs in the DPU set
  DPU_ASSERT(
      dpu_get_nr_dpus(dpu_set, &nr_of_ranks)); // Number of ranks in the DPU set
  printf("Allocated %d DPU(s), %d rank(s)\r\n", nr_of_dpus, nr_of_ranks);
  printf("NR_TASKLETS\t%d\tNR_ELEM\t%d\n", NR_TASKLETS, NR_ELEM);

  uint32_t cycles_retrieve[nr_of_dpus];
  double cycles_res[nr_of_dpus];
  memset(cycles_res, 0, nr_of_dpus * sizeof(cycles_res[0]));
  // Load binary
  DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY, NULL));

  // Input size - number of pixels
  const unsigned int nr_pixels = p.n_pixels;
  const unsigned int nr_pixels_dpu = divceil(nr_pixels, nr_of_dpus);
  const unsigned int input_size_dpu_8bytes =
      roundup(nr_pixels_dpu * sizeof(pixel_t), 24);
  const unsigned int nr_pixels_dpu_8bytes =
      input_size_dpu_8bytes / sizeof(pixel_t); // for comparing with host

  printf("nr_pixels: %u, nr_pixels_dpu: %u, input_size_dpu_8bytes: %u, "
         "nr_pixels_dpu_8bytes: %u\r\n",
         nr_pixels, nr_pixels_dpu, input_size_dpu_8bytes, nr_pixels_dpu_8bytes);
  printf("last dpu input size: %u\r\n",
         nr_pixels - nr_pixels_dpu * (nr_of_dpus - 1));
  // Input/output allocation in host main memory

  Y = malloc(nr_of_dpus * input_size_dpu_8bytes);
  Y_host = malloc(nr_of_dpus * input_size_dpu_8bytes);
  pixel_t *bufferY = Y;
  unsigned int i = 0;

  // Create an input file with arbitrary data
  read_input(Y, nr_pixels);
  memcpy(Y_host, Y, nr_of_dpus * input_size_dpu_8bytes);

  // Loop over main kernel
  for (int rep = 0; rep < p.n_warmup + p.n_reps; rep++) {

    // Compute output on CPU (verification purposes)
    if (rep >= p.n_warmup)
      start(&timer, 0, rep - p.n_warmup);
    grayscale_host(Y_host, nr_pixels);
    if (rep >= p.n_warmup)
      stop(&timer, 0);

    // Input arguments
    unsigned int kernel = 0;
    dpu_arguments_t input_arguments[NR_DPUS];
    for (i = 0; i < nr_of_dpus - 1; i++) {
      input_arguments[i].size = nr_pixels_dpu * sizeof(pixel_t);
      input_arguments[i].kernel = kernel;
      input_arguments[i].transfer_size = input_size_dpu_8bytes;
    }
    input_arguments[nr_of_dpus - 1].size =
        (nr_pixels - nr_pixels_dpu * (nr_of_dpus - 1)) * sizeof(pixel_t);
    input_arguments[nr_of_dpus - 1].transfer_size = input_size_dpu_8bytes;
    input_arguments[nr_of_dpus - 1].kernel = kernel;

    if (rep >= p.n_warmup)
      start(&timer, 1, rep - p.n_warmup); // Start timer (CPU-DPU transfers)
    // Copy input arguments
    // Parallel transfers
    DPU_FOREACH(dpu_set, dpu, i) {
      DPU_ASSERT(dpu_prepare_xfer(dpu, &input_arguments[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS", 0,
                             sizeof(input_arguments[0]), DPU_XFER_DEFAULT));

    // Copy input arrays
#ifdef SERIAL // Serial transfers
    DPU_FOREACH(dpu_set, dpu, i) {
      DPU_ASSERT(dpu_copy_to(dpu, DPU_MRAM_HEAP_POINTER_NAME, 0,
                             bufferY + nr_pixels_dpu_8bytes * i,
                             input_size_dpu_8bytes));
    }

    // DPU_FOREACH(dpu_set, dpu, i) {
    //   DPU_ASSERT(dpu_copy_to(
    //       dpu, DPU_MRAM_HEAP_POINTER_NAME, input_size_dpu_8bytes,
    //       bufferY + nr_pixels_dpu_8bytes * i, input_size_dpu_8bytes));
    // }
#else // Parallel transfers
    DPU_FOREACH(dpu_set, dpu, i) {
      DPU_ASSERT(dpu_prepare_xfer(dpu, bufferY + nr_pixels_dpu_8bytes * i));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU,
                             DPU_MRAM_HEAP_POINTER_NAME, 0,
                             input_size_dpu_8bytes, DPU_XFER_DEFAULT));

    // DPU_FOREACH(dpu_set, dpu, i) {
    //   DPU_ASSERT(dpu_prepare_xfer(dpu, bufferY + nr_pixels_dpu_8bytes * i));
    // }
    // DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU,
    //                          DPU_MRAM_HEAP_POINTER_NAME,
    //                          input_size_dpu_8bytes, input_size_dpu_8bytes,
    //                          DPU_XFER_DEFAULT));

#endif
    if (rep >= p.n_warmup)
      stop(&timer, 1); // Stop timer (CPU-DPU transfers)

    // Run DPU kernel
    if (rep >= p.n_warmup) {
      start(&timer, 2, rep - p.n_warmup); // Start timer (DPU kernel)
    }
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    if (rep >= p.n_warmup) {
      stop(&timer, 2); // Stop timer (DPU kernel)
    }

#if PRINT
    {
      unsigned int each_dpu = 0;
      printf("Display DPU Logs\n");
      DPU_FOREACH(dpu_set, dpu) {
        printf("DPU#%d:\n", each_dpu);
        DPU_ASSERT(dpulog_read_for_dpu(dpu.dpu, stdout));
        each_dpu++;
      }
    }
#endif

    if (rep >= p.n_warmup)
      start(&timer, 3, rep - p.n_warmup); // Start timer (DPU-CPU transfers)
    i = 0;
    // Copy output array
#ifdef SERIAL // Serial transfers

    //@@ INSERT SERIAL DPU-CPU TRANSFER HERE
    DPU_FOREACH(dpu_set, dpu, i) {
      DPU_ASSERT(dpu_copy_from(dpu, DPU_MRAM_HEAP_POINTER_NAME, 0,
                               bufferY + nr_pixels_dpu_8bytes * i,
                               input_size_dpu_8bytes));
    }

#else // Parallel transfers

    //@@ INSERT PARALLEL DPU-CPU TRANSFER HERE
    DPU_FOREACH(dpu_set, dpu, i) {
      DPU_ASSERT(dpu_prepare_xfer(dpu, bufferY + nr_pixels_dpu_8bytes * i));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU,
                             DPU_MRAM_HEAP_POINTER_NAME, 0,
                             input_size_dpu_8bytes, DPU_XFER_DEFAULT));

#endif
    if (rep >= p.n_warmup)
      stop(&timer, 3); // Stop timer (DPU-CPU transfers)

#if defined(CYCLES) || defined(INSTRUCTIONS)
    dpu_results_t results[nr_of_dpus];
    // Parallel transfers
    dpu_results_t *results_retrieve[nr_of_dpus];
    DPU_FOREACH(dpu_set, dpu, i) {
      results_retrieve[i] =
          (dpu_results_t *)malloc(NR_TASKLETS * sizeof(dpu_results_t));
      DPU_ASSERT(dpu_prepare_xfer(dpu, results_retrieve[i]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, "DPU_RESULTS", 0,
                             NR_TASKLETS * sizeof(dpu_results_t),
                             DPU_XFER_DEFAULT));
    DPU_FOREACH(dpu_set, dpu, i) {
      results[i].count = 0;
      // Retrieve tasklet count
      for (unsigned int each_tasklet = 0; each_tasklet < NR_TASKLETS;
           each_tasklet++) {
        if (results_retrieve[i][each_tasklet].count > results[i].count)
          results[i].count = results_retrieve[i][each_tasklet].count;
      }
      free(results_retrieve[i]);
    }

    uint64_t max_count = 0;
    uint64_t min_count = 0xFFFFFFFFFFFFFFFF;
    // Print performance results
    if (rep >= p.n_warmup) {
      i = 0;
      DPU_FOREACH(dpu_set, dpu) {
        if (results[i].count > max_count)
          max_count = results[i].count;
        if (results[i].count < min_count)
          min_count = results[i].count;
        i++;
      }
      cc += (double)max_count;
      cc_min += (double)min_count;
    }
#endif
    // use CLK speed to detect DPU types
    if (rep >= p.n_warmup) {
      DPU_FOREACH(dpu_set, dpu, i) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, &cycles_retrieve[i]));
      }
      DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, "CLOCKS_PER_SEC", 0,
                               sizeof(uint32_t), DPU_XFER_DEFAULT));
      DPU_FOREACH(dpu_set, dpu, i) { cycles_res[i] += cycles_retrieve[i]; }
    }
  }
  for (i = 0; i < nr_of_dpus; ++i)
    printf("DPU %d clocks_per_sec  = %g\n", i, cycles_res[i] / (p.n_reps));
#ifdef CYCLES
  printf("DPU cycles  = %g\n", cc / p.n_reps);
#elif INSTRUCTIONS
  printf("DPU instructions  = max: %.0f, diff: %.12g\n",
         cc / (p.n_reps * NR_TASKLETS), (cc - cc_min) / p.n_reps);
  printf("Arithmetic throughput = %g MOP/s\n", cc / (timer.time[2]));
#endif

  // Print timing results
  printf("CPU ");
  print(&timer, 0, p.n_reps);
  printf("CPU-DPU ");
  print(&timer, 1, p.n_reps);
  printf("DPU Kernel ");
  print(&timer, 2, p.n_reps);
  printf("DPU-CPU ");
  print(&timer, 3, p.n_reps);

  double time_cpu = timer.time[1] / (1e6 * p.n_reps);
  double time_kernel = timer.time[2] / (1e6 * p.n_reps);
  double time_dpu = timer.time[3] / (1e6 * p.n_reps);
  printf("\r\nCPU-DPU bandwidth (GB/s): %f\r\n",
         (nr_pixels * 8) / (time_cpu * (1 << 30)));
  printf("DPU kernels bandwidth (GB/s): %f\r\n",
         (nr_pixels * 8) / (time_kernel * (1 << 30)));
  printf("DPU-CPU bandwidth (GB/s): %f\r\n",
         (nr_pixels * 8) / (time_dpu * (1 << 30)));
  // Check output
  bool status = true;
  for (i = 0; i < nr_of_dpus; i++) {
    for (unsigned int j = 0; j < nr_pixels_dpu; j++) {
      if (Y_host[i * nr_pixels_dpu_8bytes + j].r !=
              Y[i * nr_pixels_dpu_8bytes + j].r ||
          Y_host[i * nr_pixels_dpu_8bytes + j].g !=
              Y[i * nr_pixels_dpu_8bytes + j].g ||
          Y_host[i * nr_pixels_dpu_8bytes + j].b !=
              Y[i * nr_pixels_dpu_8bytes + j].b) {
        status = false;
        printf("%u: %u -- %u\n", i * nr_pixels_dpu_8bytes + j,
               Y_host[i * nr_pixels_dpu_8bytes + j].r,
               Y[i * nr_pixels_dpu + j].r);
        printf("%u: %u -- %u\n", i * nr_pixels_dpu_8bytes + j,
               Y_host[i * nr_pixels_dpu_8bytes + j].g,
               Y[i * nr_pixels_dpu + j].g);
        printf("%u: %u -- %u\n", i * nr_pixels_dpu_8bytes + j,
               Y_host[i * nr_pixels_dpu_8bytes + j].b,
               Y[i * nr_pixels_dpu + j].b);
      }
    }
  }

  if (status) {
    printf("[" ANSI_COLOR_GREEN "OK" ANSI_COLOR_RESET "] Outputs are equal\n");
  } else {
    printf("[" ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET "] Outputs differ!\n");
  }

  // Deallocation
  // free(X);
  free(Y);
  free(Y_host);
  DPU_ASSERT(dpu_free(dpu_set)); // Deallocate DPUs

  return status ? 0 : -1;
}
