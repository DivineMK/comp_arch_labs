#ifndef _CACHE_H_
#define _CACHE_H_

#include "shell.h"

#ifndef CACHE_INST_SIZE
#define CACHE_INST_SIZE 8192
#endif

#ifndef CACHE_INST_BLOCK_SIZE
#define CACHE_INST_BLOCK_SIZE 32
#endif

#ifndef CACHE_INST_ASSO
#define CACHE_INST_ASSO 4
#endif

#ifndef CACHE_DATA_SIZE
#define CACHE_DATA_SIZE 65536
#endif

#ifndef CACHE_DATA_BLOCK_SIZE
#define CACHE_DATA_BLOCK_SIZE 32
#endif

#ifndef CACHE_DATA_ASSO
#define CACHE_DATA_ASSO 8
#endif

extern uint32_t stall_cycles;
extern uint32_t miss_data;

typedef struct {
  uint32_t *cache_line;
  uint32_t tag;
  uint32_t recency;
  uint32_t valid;
  uint32_t dirty;
} cache_line_t;

typedef struct {
  cache_line_t *cached_array;
  uint32_t cache_size;
  uint32_t block_size;
  uint8_t associativity;
  uint8_t n_bit_cache;
  uint8_t n_bit_block;
  uint8_t n_bit_assoc;
} cache_t;

typedef struct {
  uint32_t data;
  uint8_t hit;
} cache_result_t;

void init_cache(cache_t *cache, uint32_t cache_size, uint32_t block_size,
                uint8_t associativity);

cache_result_t read_cache(cache_t *cache, uint32_t addr);
uint8_t write_cache(cache_t *cache, uint32_t addr, uint32_t data);

uint32_t get_index(cache_t *cache, uint32_t addr);
uint32_t get_tag(cache_t *cache, uint32_t addr);
uint32_t get_block_offset(cache_t *cache, uint32_t addr);

// log2 func to count number of bits needed for val
uint8_t ilog2(uint32_t val);
#endif
