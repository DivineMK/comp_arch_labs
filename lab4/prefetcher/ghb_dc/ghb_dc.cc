#include "ghb_dc.h"
#include "champsim.h"
#include "dpc_api.h"

#include "cache.h"

static uint16_t cycle_cnt = 0;
static uint8_t start = 1;
static champsim::block_number cl_addr_old;

template <typename T> T unsigned_sub(T a, T b, T limit) {
  return (a >= b) ? (a - b) : (limit - (b - a));
}

uint32_t
ghb_dc::prefetcher_cache_operate(champsim::address addr, champsim::address ip,
                                 uint8_t cache_hit, bool useful_prefetch,
                                 access_type type, uint32_t metadata_in) {

  champsim::block_number cl_addr{addr};
  if (start == 1) {
    cl_addr_old = cl_addr;
    start = 0;
    return 0;
  }
  // champsim::offset(a, b) = b - a
  champsim::block_number::difference_type delta =
      champsim::offset(cl_addr_old, cl_addr);
  cl_addr_old = cl_addr;

  if (delta == 0)
    return 0;

  size_t it_idx, ghb_idx = 0;
  // index by both delta
  // FIFO replacement
  bool key_in_it = false;
  if (index_table.size > 0) {
    for (size_t i = 1; i <= index_table.size; i++) {
      it_idx = unsigned_sub(index_table.head, i, index_table.size);
      if (index_table.buf[it_idx].delta == delta) {
        key_in_it = true;
        break;
      }
    }
  }

  if (!key_in_it) {
    index_table.push(delta, ghb.head);
    ghb.push(delta, addr, GHB_SIZE); // GHB_SIZE == null
    return metadata_in;
  }

  // check if ghb_idx point to itself or point to entry with different delta
  if (ghb.head == index_table.buf[it_idx].ghb_idx ||
      delta != ghb.buf[index_table.buf[it_idx].ghb_idx].delta)
    ghb_idx = GHB_SIZE;
  else {
    ghb_idx = index_table.buf[it_idx].ghb_idx;
  }

  index_table.buf[it_idx].ghb_idx = ghb.head;

  size_t ghb_idx_new = ghb.head, ghb_idx_old = ghb_idx;

  ghb.push(delta, addr, ghb_idx);

  size_t pref_cnt_width{0};
  size_t ghb_tmp = 0;

  // have a counter to count remaining n0 prefetches
  size_t counter = PREFETCH_DEPTH * PREFETCH_WIDTH;
  prefetches_head = 0;
  while (counter > 0 && ghb_idx_old != GHB_SIZE) {

    ghb_tmp = unsigned_sub(ghb_idx_new, ghb_idx_old, GHB_SIZE);
    pref_cnt_width = (ghb_tmp <= counter) ? ghb_tmp : counter;
    pref_cnt_width = 1;

    for (size_t i = 1; i < pref_cnt_width; i++) {
      // if (prefetches_head > 0 &&
      //     champsim::page_number{prefetches[prefetches_head - 1]} ==
      //         champsim::page_number{ghb.buf[ghb_idx_old + i].cl_addr})
      cl_addr += ghb.buf[(ghb_idx_old + i) % GHB_SIZE].delta;
      prefetches[prefetches_head++] = champsim::address{cl_addr};
      // fprintf(stderr, "test: %ld\r\n",
      //         champsim::address{champsim::block_number{0} +
      //                           champsim::block_number::difference_type{1}});
      // std::abort();
    }
    // TODO: what happens when pref_cnt_width == counter
    if (pref_cnt_width <= counter) {
      cl_addr += ghb.buf[ghb_idx_old].delta;
      prefetches[prefetches_head++] = champsim::address{cl_addr};
    }
    counter -= pref_cnt_width;
    // check next starting point
    ghb_idx_new = ghb_idx_old;
    ghb_idx_old = ghb.buf[ghb_idx_old].prev;
  }

  return metadata_in;
}
void ghb_dc::prefetcher_cycle_operate() {
  // check if any prefetches available
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
      // TODO: consider using success
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
