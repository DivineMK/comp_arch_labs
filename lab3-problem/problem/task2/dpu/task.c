/*
 * AXPY with multiple tasklets
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

// AXPY: Computes AXPY for a cached block
static void axpy(T *bufferY, T *bufferX, T alpha, unsigned int l_size) {

  //@@ INSERT AXPY CODE
  for (unsigned int i = 0; i < l_size; i++) {
    bufferY[i] += alpha * bufferX[i];
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
  counter_start(&count); // START TIMER
#endif

  uint32_t input_size_dpu_bytes =
      DPU_INPUT_ARGUMENTS.size; // Input size per DPU in bytes
  uint32_t input_size_dpu_bytes_transfer =
      DPU_INPUT_ARGUMENTS.transfer_size; // Transfer input size per DPU in bytes
  T alpha = DPU_INPUT_ARGUMENTS.alpha;   // alpha (a in axpy)

  // Address of the current processing block in MRAM
  uint32_t base_tasklet = tasklet_id << BLOCK_SIZE_LOG2;
  uint32_t mram_base_addr_X = (uint32_t)DPU_MRAM_HEAP_POINTER;
  uint32_t mram_base_addr_Y =
      (uint32_t)(DPU_MRAM_HEAP_POINTER + input_size_dpu_bytes_transfer);

  // Initialize a local cache in WRAM to store the MRAM block
  //@@ INSERT WRAM ALLOCATION HERE
  T *cacheX = (T *)mem_alloc(BLOCK_SIZE);
  T *cacheY = (T *)mem_alloc(BLOCK_SIZE);

// distribute data to every tasklet -> separated by BLOCK_SIZE * NR_TASKLETS
#if PRINT
  uint32_t iter_cnt = 0;
  uint64_t test = 0, start = 0;
#endif
  for (uint32_t byte_index = base_tasklet; byte_index < input_size_dpu_bytes;
       byte_index += BLOCK_SIZE * NR_TASKLETS) {

#if PRINT
    iter_cnt++;
#endif
    // Bound checking
    //@@ INSERT BOUND CHECKING HERE
    uint32_t l_size_bytes = (byte_index + BLOCK_SIZE >= input_size_dpu_bytes)
                                ? (input_size_dpu_bytes - byte_index)
                                : BLOCK_SIZE;

    // Load cache with current MRAM block
    //@@ INSERT MRAM-WRAM TRANSFERS HERE
    mram_read((__mram_ptr void const *)(mram_base_addr_X + byte_index), cacheX,
              l_size_bytes);
    mram_read((__mram_ptr void const *)(mram_base_addr_Y + byte_index), cacheY,
              l_size_bytes);

    // barrier synchronization can timeout leading to crash -> output error
    // barrier_wait(&my_barrier);
    //
    // Computer vector addition
    //@@ INSERT CALL TO AXPY FUNCTION HERE
    axpy(cacheY, cacheX, alpha, l_size_bytes >> DIV);

#if defined(CYCLES) || defined(INSTRUCTIONS)
    result->count = counter_stop(&count); // STOP TIMER
#endif
    // Write cache to current MRAM block
    //@@ INSERT WRAM-MRAM TRANSFER HERE
#if PRINT
    start = perfcounter_get();
#endif
    mram_write(cacheY, (__mram_ptr void *)(mram_base_addr_Y + byte_index),
               l_size_bytes);
#if PRINT
    test = perfcounter_get() - start;
#endif
  }
#if PRINT
  printf("tasklet [%u] iter_cnt: %u\t count: %lu\t test: %lu\r\n", tasklet_id,
         iter_cnt, DPU_RESULTS[tasklet_id].count, test);
#endif

  return 0;
}
