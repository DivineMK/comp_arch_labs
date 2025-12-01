#ifndef _CACHE_H_
#define _CACHE_H_

#include "shell.h"

#define CACHE_INST_SIZE 8192
#define CACHE_INST_BLOCK_SIZE 32
#define CACHE_INST_ASSO 4

#define CACHE_DATA_SIZE 65536
#define CACHE_DATA_BLOCK_SIZE 32
#define CACHE_DATA_ASSO 8

extern uint32_t stall_cycles;
extern uint32_t stall_pos;

typedef struct {
  uint32_t *cache_line;
  uint8_t tag;
  uint8_t recency;
  uint8_t valid;
  uint8_t dirty;
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
cache_result_t write_cache(cache_t *cache, uint32_t addr, uint32_t data);

uint32_t get_index(cache_t *cache, uint32_t addr);
uint32_t get_tag(cache_t *cache, uint32_t addr);
uint32_t get_block_offset(cache_t *cache, uint32_t addr);

// log2 func to count number of bits needed for val
uint8_t ilog2(uint32_t val);
#endif
