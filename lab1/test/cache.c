#include "cache.h"
#include "shell.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint32_t stall_cycles = 0;
uint32_t stall_pos = 0;

void init_cache(cache_t *cache, uint32_t cache_size, uint32_t block_size,
                uint8_t associativity) {
  cache->cache_size = cache_size;
  cache->block_size = block_size / 4; // change unit to 32 bit
  cache->associativity = associativity;
  cache->n_bit_cache = ilog2(cache->cache_size);
  cache->n_bit_block = ilog2(block_size);
  cache->n_bit_assoc = ilog2(cache->associativity);

  cache->cached_array = malloc(sizeof(cache_line_t) * cache_size / block_size);
  memset(cache->cached_array, 0,
         sizeof(cache_line_t) * cache_size / block_size);
  for (uint32_t i = 0; i < cache_size / block_size; i++) {
    cache->cached_array[i].cache_line = malloc(sizeof(uint32_t) * block_size);
    memset(cache->cached_array[i].cache_line, 0, sizeof(uint32_t) * block_size);
  }
}

uint32_t get_index(cache_t *cache, uint32_t addr) {
  return (addr & ((1 << (cache->n_bit_cache - cache->n_bit_assoc)) - 1)) >>
         cache->n_bit_block;
}

uint32_t get_tag(cache_t *cache, uint32_t addr) {
  return addr >> (cache->n_bit_cache - cache->n_bit_assoc);
}

uint32_t get_block_offset(cache_t *cache, uint32_t addr) {
  return (addr & ((1 << cache->n_bit_block) - 1)) >> 2;
}

cache_result_t read_cache(cache_t *cache, uint32_t addr) {
  uint32_t set_idx = get_index(cache, addr);
  uint32_t tag = get_tag(cache, addr);
  uint32_t block_offset = get_block_offset(cache, addr);

  cache_result_t res = {0};
  uint32_t line = set_idx;
  // cache hit
  while (line < (set_idx + cache->associativity)) {
    if (cache->cached_array[line].tag == tag) {
      res.data = cache->cached_array[line].cache_line[block_offset];
      res.hit = 1;
      return res;
    }
    line += 1;
  }
  // cache miss
  line = set_idx;
  while (line < (set_idx + cache->associativity)) {
    // replacement: least recently used
    if (cache->cached_array[line].recency == cache->associativity - 1) {
      // write back
      if (cache->cached_array[line].dirty == 1) {
        uint32_t mem_write_addr = 0;
        for (uint32_t block = 0; block < cache->block_size / 4; block++) {
          mem_write_addr = (cache->cached_array[line].tag
                            << (cache->n_bit_cache - cache->n_bit_assoc)) |
                           (line << cache->n_bit_block) | (block << 2);
          mem_write_32(mem_write_addr,
                       cache->cached_array[line].cache_line[block]);
        }
      }
    }
    // check empty cache lines
    if (cache->cached_array[line].recency == cache->associativity - 1 ||
        cache->cached_array[line].valid == 0) {
      uint32_t mem_read_addr = 0;
      for (uint32_t block = 0; block < cache->block_size; block++) {
        // mem_addr = (tag << (cache->n_bit_cache - cache->n_bit_assoc) | (line
        // << cache->n_bit_block) | (block << 2);
        mem_read_addr =
            (addr & ~((1 << cache->n_bit_block) - 1)) | (block << 2);
        cache->cached_array[line].cache_line[block] =
            mem_read_32(mem_read_addr);
      }
      cache->cached_array[line].valid = 1;
      cache->cached_array[line].dirty = 0;
      cache->cached_array[line].tag = tag;
      cache->cached_array[line].recency = 0;
      uint32_t j = set_idx;
      while (j < set_idx + cache->associativity) {
        if (j == line)
          continue;
        cache->cached_array[j].recency++;
        j++;
      }
      res.data = cache->cached_array[line].cache_line[block_offset];
      return res;
    }
    line += 1;
  }

  return res;
}

uint8_t ilog2(uint32_t val) {
  uint8_t res = 0;
  while (val >>= 1)
    res++;
  return res;
}

int main(void) {
  cache_t cache_inst;
  init_cache(&cache_inst, CACHE_INST_SIZE, CACHE_INST_BLOCK_SIZE,
             CACHE_INST_ASSO);
  uint32_t addr = 0b10101010101010101010110101110100;
  printf("%b\r\n", (1 << cache_inst.n_bit_block) - 1);
  printf("bit_cache:%u\r\n", cache_inst.n_bit_cache);
  printf("bit_block:%u\r\n", cache_inst.n_bit_block);
  printf("bit_assoc:%u\r\n", cache_inst.n_bit_assoc);
  printf("addr:%b\r\n", addr);
  printf("tag:%b\r\n", get_tag(&cache_inst, addr));
  printf("index:%b\r\n", get_index(&cache_inst, addr));
  printf("block:%b\r\n", get_block_offset(&cache_inst, addr));
  return EXIT_SUCCESS;
}
