/*
 * Arithmetic throughput for ADD, SUB, DIV, MUL
 *
 */
#include <alloc.h>
#include <barrier.h>
#include <defs.h>
#include <mram.h>
#include <perfcounter.h>
#include <stdint.h>
#include <stdio.h>

#include "../support/common.h"
#include "../support/cyclecount.h"

// Input and output arguments
__host dpu_arguments_t DPU_INPUT_ARGUMENTS;
__host dpu_results_t DPU_RESULTS[NR_TASKLETS];

// Barrier
BARRIER_INIT(my_barrier, NR_TASKLETS);

extern int main_kernel1(void);
int (*kernels[nr_kernels])(void) = {main_kernel1};
int main(void) {
  // Kernel
  return kernels[DPU_INPUT_ARGUMENTS.kernel]();
}

static void grayscale(pixel_t *A, unsigned int l_size) {
  uint8_t grayscale;
#pragma unroll
  for (unsigned int i = 0; i < l_size; i++) {
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

// main_kernel1
int main_kernel1() {
  unsigned int tasklet_id = me();
#if PRINT
  printf("tasklet_id = %u\n", tasklet_id);
  printf("tasklet [%u] stack size: %u\r\n", tasklet_id, check_stack());
#endif
  if (tasklet_id == 0) {
    mem_reset(); // Reset the heap
#ifdef CYCLES
    perfcounter_config(COUNT_CYCLES, true); // Initialize once the cycle counter
#elif INSTRUCTIONS
    perfcounter_config(COUNT_INSTRUCTIONS,
                       true); // Initialize once the instruction counter
#endif
  }
  // Barrier
  barrier_wait(&my_barrier);
#if defined(CYCLES) || defined(INSTRUCTIONS)
  perfcounter_count count;
  dpu_results_t *result = &DPU_RESULTS[tasklet_id];
  result->count = 0;
#endif

  uint32_t input_size = DPU_INPUT_ARGUMENTS.size; // Input size per DPU in bytes
  // uint32_t transfer_size = DPU_INPUT_ARGUMENTS.transfer_size; // 8-byte
  // aligned

  // Address of the current processing block in MRAM
  uint32_t pixel_block = NR_ELEM * sizeof(pixel_t);
  if (pixel_block < 8 || pixel_block > 2048) {
    printf("Invalid MRAM transfer size %d, must be 8 <= x <= 2048\r\n",
           pixel_block);
    return 1;
  }
  if (pixel_block % 24 != 0 && pixel_block > input_size) {
    printf("pixel_block need to be byte-aligned and div sizeof(pixel_t) = 0 -> "
           "div 24 = 0\r\n");
    return 1;
  }

  uint32_t base_tasklet = tasklet_id * pixel_block;
  uint32_t mram_base_addr_Y = (uint32_t)DPU_MRAM_HEAP_POINTER;
  // uint32_t mram_base_addr_Y = (uint32_t)(DPU_MRAM_HEAP_POINTER +
  // transfer_size); Initialize a local cache in WRAM to store the MRAM block
  pixel_t *cacheY = (pixel_t *)mem_alloc(pixel_block);
  // pixel_t *cacheY = (pixel_t *)mem_alloc(pixel_block);

  uint32_t nr_elems;
  // distribute data to every tasklet -> separated by NR_ELEM * NR_TASKLETS
  for (unsigned int byte_index = base_tasklet; byte_index < input_size;
       byte_index += pixel_block * NR_TASKLETS) {

    // Bound checking
    uint32_t size_pixels = (byte_index + pixel_block >= input_size)
                               ? (input_size - byte_index)
                               : pixel_block;

    if (size_pixels % 8 != 0) {
      nr_elems = size_pixels / sizeof(pixel_t);
      size_pixels = roundup(size_pixels, 8);
    } else
      nr_elems = NR_ELEM;

    // Load cache with current MRAM block
    mram_read((__mram_ptr void const *)(mram_base_addr_Y + byte_index), cacheY,
              size_pixels);
    // mram_read((__mram_ptr void const *)(mram_base_addr_Y + byte_index),
    // cacheY, size_pixels);

#if defined(CYCLES) || defined(INSTRUCTIONS)
    counter_start(&count); // START TIMER
#endif
    grayscale(cacheY, nr_elems);

#if defined(CYCLES) || defined(INSTRUCTIONS)
    result->count += counter_stop(&count); // STOP TIMER
#endif

    // Write cache to current MRAM block
    mram_write(cacheY, (__mram_ptr void *)(mram_base_addr_Y + byte_index),
               size_pixels);
  }

  return 0;
}
