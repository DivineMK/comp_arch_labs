#include "ghb_dc.h"
#include "champsim.h"
#include "dpc_api.h"

#include "cache.h"
#include <array>
#include <cstdint>
#include <sys/types.h>
#include <vector>

uint16_t cycle_cnt = 0;
uint8_t start = 1;
champsim::address miss_addr_old;

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

  ghb_idx = index_table.buf[it_idx].ghb_idx;
  index_table.buf[it_idx].ghb_idx = ghb.head;
  ghb.push(delta, ghb_idx);

  // start from prev ghb entry
  champsim::block_number::difference_type delta_cur{ghb.buf[ghb_idx].delta},
      delta_prev;

  size_t pref_cnt_width{0};
  std::array<champsim::address, PREFETCH_DEPTH * PREFETCH_WIDTH> prefetches;
  uint8_t prefetches_head{0};
  size_t ghb_idx_prev = ghb.head - 1;

  size_t ghb_tmp = 0;

  while (counter > 0) {
    // check next starting point
    ghb_idx = ghb.buf[ghb_idx].prev;

    ghb_tmp = (ghb_idx_prev > ghb_idx) ? (ghb_idx_prev - ghb_idx)
                                       : (GHB_SIZE - (ghb_idx - ghb_idx_prev));
    assert(ghb_tmp < GHB_SIZE);

    if (ghb_idx != GHB_SIZE) {
      pref_cnt_width = (ghb_tmp < counter) ? ghb_tmp : counter;
      assert(pref_cnt_width < GHB_SIZE);

      for (size_t i = pref_cnt_width; i > 0; i--) {
        // where to start??
        prefetches[prefetches_head] = ghb.buf[ghb_idx - i];
        prefetches_head++;
      }
      counter -= pref_cnt_width;
    }
    counter--;
  }

  return metadata_in;
}
void ghb_dc::prefetcher_cycle_operate() {
  // if (cycle_cnt < 1000)
  //   cycle_cnt++;
  // else {
  //   cycle_cnt = 0;
  //   adjust_degree();
  // }

  // If a lookahead is active
  if (active_lookahead.has_value()) {
    auto [old_pf_address, stride, degree] = active_lookahead.value();
    assert(degree > 0);
    if (stride == 0)
      return;

    champsim::address pf_address{champsim::block_number{old_pf_address} +
                                 stride};

    // If the next step would exceed the degree or run off the page, stop
    if (intern_->virtual_prefetch ||
        champsim::page_number{pf_address} ==
            champsim::page_number{old_pf_address}) {
      // check the MSHR occupancy to decide if we're going to prefetch to this
      // level or not
      const bool mshr_under_light_load =
          intern_->get_mshr_occupancy_ratio() < 0.5;
      // const bool mshr_under_light_load = true;
      const bool success = prefetch_line(pf_address, mshr_under_light_load, 0);
      if (success)
        active_lookahead = {pf_address, stride, degree - 1};
      // If we fail, try again next cycle

      if (active_lookahead->degree == 0) {
        active_lookahead.reset();
      }
    } else {
      active_lookahead.reset();
    }
  }
}

uint32_t ghb_dc::prefetcher_cache_fill(champsim::address addr, long set,
                                       long way, uint8_t prefetch,
                                       champsim::address evicted_addr,
                                       uint32_t metadata_in) {
  return metadata_in;
}
