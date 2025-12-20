#include "ghb_dc.h"
#include "champsim.h"
#include "dpc_api.h"

#include "cache.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sys/types.h>

static uint16_t cycle_cnt = 0;
uint8_t start = 1;
champsim::address miss_addr_old;

template <typename T> T unsigned_sub(T a, T b, T limit) {
  return (a >= b) ? (a - b) : (limit - (b - a));
}

uint32_t
ghb_dc::prefetcher_cache_operate(champsim::address addr, champsim::address ip,
                                 uint8_t cache_hit, bool useful_prefetch,
                                 access_type type, uint32_t metadata_in) {

  if (start == 1) {
    miss_addr_old = addr;
    start = 0;
    return 0;
  }

  champsim::block_number::difference_type delta =
      champsim::offset(addr, miss_addr_old);

  size_t it_idx, ghb_idx = 0;
  // index by both delta and ip
  // FIFO replacement
  bool key_in_it = false;
  if (index_table.size > 0) {
    for (size_t i = 1; i <= index_table.size; i++) {
      it_idx = (index_table.head - i) % index_table.size;
      if (index_table.buf[it_idx].ip == ip &&
          index_table.buf[it_idx].delta == delta) {
        key_in_it = true;
        break;
      }
    }
  }

  if (!key_in_it) {
    index_table.push(ip, delta, ghb.head);
    ghb.push(delta, GHB_SIZE);
    return metadata_in;
  }

  // have a counter to count n0 prefetches (to match ip_stride cases)
  // (optional) have a set to store next prefetches
  size_t counter = PREFETCH_DEPTH * PREFETCH_WIDTH;
  prefetches_head = 0;

  ghb_idx = index_table.buf[it_idx].ghb_idx;
  index_table.buf[it_idx].ghb_idx = ghb.head;
  ghb.push(delta, ghb_idx);

  size_t pref_cnt_width{0};
  size_t ghb_idx_cur = ghb.head - 1; // ghb.head >= 1
  size_t ghb_tmp = 0;

  champsim::block_number cl_addr = champsim::block_number{addr};
  while (counter > 0 && ghb_idx != GHB_SIZE) {
    ghb_tmp = unsigned_sub(ghb_idx_cur, ghb_idx, GHB_SIZE);

    if (ghb_tmp >= GHB_SIZE) {
      fprintf(stderr, "ghb_tmp: %ld, ghb_idx: %ld, ghb_idx_prev: %ld\r\n",
              ghb_tmp, ghb_idx, ghb_idx_cur);
      std::abort();
    }

    pref_cnt_width = (ghb_tmp <= counter) ? ghb_tmp : counter;

    if (pref_cnt_width >= GHB_SIZE) {
      fprintf(stderr, "pref_cnt_width: %ld, counter: %ld\r\n", pref_cnt_width,
              counter);
      std::abort();
    }

    // pref_cnt_width >= 1
    for (size_t i = 1; i < pref_cnt_width; i++) {
      prefetches[prefetches_head++] =
          champsim::address{cl_addr + ghb.buf[ghb_idx + i].delta};
      // fprintf(stderr, "test: %ld\r\n",
      //         champsim::address{champsim::block_number{0} +
      //                           champsim::block_number::difference_type{1}});
      // std::abort();
    }
    prefetches[prefetches_head++] =
        champsim::address{cl_addr + ghb.buf[ghb_idx].delta};
    counter -= pref_cnt_width;
    // check next starting point
    ghb_idx = ghb.buf[ghb_idx].prev;
  }

  return metadata_in;
}
void ghb_dc::prefetcher_cycle_operate() {
  // If a lookahead is active
  if (prefetches_head == 0)
    return;

  champsim::address old_pf_address{prefetches[0]};
  for (uint8_t i = 0; i < prefetches_head; i++) {
    // If the next step would exceed the degree or run off the page, stop
    if (intern_->virtual_prefetch ||
        champsim::page_number{prefetches[i]} ==
            champsim::page_number{old_pf_address}) {
      // check the MSHR occupancy to decide if we're going to prefetch to this
      // level or not
      // const bool mshr_under_light_load =
      //     intern_->get_mshr_occupancy_ratio() < 0.5;
      const bool mshr_under_light_load = true;
      const bool success =
          prefetch_line(prefetches[i], mshr_under_light_load, 0);
      // consider using success
      old_pf_address = prefetches[i];
    } else
      continue;
  }
}

uint32_t ghb_dc::prefetcher_cache_fill(champsim::address addr, long set,
                                       long way, uint8_t prefetch,
                                       champsim::address evicted_addr,
                                       uint32_t metadata_in) {
  return metadata_in;
}
