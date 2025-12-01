/*
 * Vector add reduction
 * Based on https://github.com/CMU-SAFARI/prim-benchmarks/blob/main/RED/
 */
#include <alloc.h>
#include <barrier.h>
#include <defs.h>
#include <handshake.h>
#include <mram.h>
#include <mutex.h>
#include <perfcounter.h>
#include <stdint.h>
#include <stdio.h>

#include "../support/common.h"
#include "../support/cyclecount.h"

// Input and output arguments
__host dpu_arguments_t DPU_INPUT_ARGUMENTS;
__host dpu_results_t DPU_RESULTS[NR_TASKLETS];

// Reduction in each tasklet
static T reduction(T *input, unsigned int l_size) {
  T ans = 0;
  for (unsigned int j = 0; j < l_size; j++) {
    ans += input[j];
  }
  return ans;
}

// Barrier
BARRIER_INIT(my_barrier, NR_TASKLETS);
#ifdef MUTEX
MUTEX_INIT(my_mutex);
T ans;
#endif /* ifdef MUTEX */

T message[NR_TASKLETS];

extern int main_kernel1(void);
int (*kernels[nr_kernels])(void) = {main_kernel1};
int main(void) {
  // Kernel
  return kernels[DPU_INPUT_ARGUMENTS.kernel]();
}

// main_kernel1
int main_kernel1() {
  unsigned int tasklet_id = me();
#if PRINT
  printf("tasklet_id = %u\n", tasklet_id);
#endif
  if (tasklet_id == 0) {
    mem_reset(); // Reset the heap
#ifdef CYCLES
    perfcounter_config(COUNT_CYCLES, true); // Initialize once the cycle counter
#elif INSTRUCTIONS
    perfcounter_config(COUNT_INSTRUCTIONS,
                       true); // Initialize once the instruction counter
#endif
#ifdef MUTEX
    ans = 0;
#endif
  }
  // Barrier
  barrier_wait(&my_barrier);

  dpu_results_t *result = &DPU_RESULTS[tasklet_id];
#if defined(CYCLES) || defined(INSTRUCTIONS)
  perfcounter_count count;
  result->count = 0;
  counter_start(&count); // START TIMER
#endif

  uint32_t input_size_dpu_bytes =
      DPU_INPUT_ARGUMENTS.size; // Input size per DPU in bytes

  // Address of the current processing block in MRAM
  uint32_t base_tasklet = tasklet_id << BLOCK_SIZE_LOG2;
  uint32_t mram_base_addr_A = (uint32_t)DPU_MRAM_HEAP_POINTER;

  // Initialize a local cache to store the MRAM block
  T *cacheA = (T *)mem_alloc(BLOCK_SIZE);

  // Local count
  T l_count = 0;
  for (unsigned int byte_index = base_tasklet;
       byte_index < input_size_dpu_bytes;
       byte_index += BLOCK_SIZE * NR_TASKLETS) {

    // Bound checking
    uint32_t l_size_bytes = (byte_index + BLOCK_SIZE >= input_size_dpu_bytes)
                                ? (input_size_dpu_bytes - byte_index)
                                : BLOCK_SIZE;

    // Load cache with current MRAM block
    mram_read((__mram_ptr void const *)(mram_base_addr_A + byte_index), cacheA,
              l_size_bytes);

    // Reduction in each tasklet
    l_count += reduction(cacheA, l_size_bytes >> DIV);
  }

  message[tasklet_id] = l_count; // save local to result array for reduction
#ifdef TREE                      // Tree-based reduction
#ifdef BARRIER
  barrier_wait(&my_barrier);
#endif

#pragma unroll
  for (unsigned int offset = 1; offset < NR_TASKLETS; offset <<= 1) {

    if ((tasklet_id & (2 * offset - 1)) == 0) {
#if !defined(BARRIER)
      // Wait
      handshake_wait_for(tasklet_id + offset);
#endif
      message[tasklet_id] += message[tasklet_id + offset];
    }

#ifdef BARRIER
    barrier_wait(&my_barrier);
#else
    else if ((tasklet_id & (offset - 1)) == 0) {
      handshake_notify();
    }
#endif
  }
#elif MUTEX
  mutex_lock(my_mutex);
  ans += message[tasklet_id];
  mutex_unlock(my_mutex);
#else // Single-thread reduction
  // Barrier
  barrier_wait(&my_barrier);
  if (tasklet_id == 0)
#pragma unroll
    for (unsigned int each_tasklet = 1; each_tasklet < NR_TASKLETS;
         each_tasklet++) {
      message[0] += message[each_tasklet];
    }
#endif

#ifdef MUTEX
  barrier_wait(&my_barrier);
#endif
  if (tasklet_id == 0) {
#ifdef MUTEX
    result->ans = ans;
#else
    result->ans = message[0];
#endif
  }
#if defined(CYCLES) || defined(INSTRUCTIONS)
  result->count += counter_stop(&count); // STOP TIMER
#endif
#if PRINT
  printf("tasklet [%u] count: %lu \r\n", tasklet_id,
         DPU_RESULTS[tasklet_id].count);
#endif

  return 0;
}
