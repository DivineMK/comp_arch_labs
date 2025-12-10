#include "ghb_stride_sa.h"
#include "dpc_api.h"

#include "cache.h"
#include <cstdint>
#include <sys/types.h>

uint16_t cycle_cnt = 0;
uint32_t ghb_stride_sa::prefetcher_cache_operate(
    champsim::address addr, champsim::address ip, uint8_t cache_hit,
    bool useful_prefetch, access_type type, uint32_t metadata_in) {

  champsim::block_number cl_addr{addr};

  size_t it_idx, ghb_idx = 0;

  // // FIFO replacement
  // bool key_in_it = false;
  // if (index_table.size > 0) {
  //   for (size_t i = 1; i <= index_table.size; i++) {
  //     it_idx = (index_table.head - i) % index_table.size;
  //     if (index_table.buf[it_idx].ip == ip) {
  //       key_in_it = true;
  //       break;
  //     }
  //   }
  // }
  //
  // if (!key_in_it) {
  //   index_table.push(ip, ghb.head);
  //   ghb.push(cl_addr, GHB_SIZE);
  //   return metadata_in;
  // }
  //
  // // FIFO replacement
  // ghb_idx = index_table.buf[it_idx].ghb_idx;
  // index_table.buf[it_idx].ghb_idx = ghb.head;
  // ////////////////////

  // LRU replacement
  auto found = index_table.check_hit({ip, ghb_idx});
  if (!found.has_value()) {
    ghb.push(cl_addr, GHB_SIZE);
    index_table.fill({ip, ghb.head});
    return metadata_in;
  }

  ghb_idx = found->ghb_idx;
  index_table.fill({ip, ghb.head});
  ////////////////////

  ghb.push(cl_addr, ghb_idx);

  champsim::block_number cl_addr_cur{ghb.buf[ghb_idx].cl_addr}, cl_addr_prev;
  champsim::block_number::difference_type tmp_stride = 0, stride = 0;

  uint8_t confidence = 0;

  while (ghb_idx != GHB_SIZE && confidence < CONFIDENCE) {
    ghb_idx = ghb.buf[ghb_idx].prev_access_id;
    if (ghb_idx == GHB_SIZE)
      return metadata_in;

    cl_addr_prev = ghb.buf[ghb_idx].cl_addr;
    tmp_stride = champsim::offset(cl_addr_prev, cl_addr_cur);

    cl_addr_prev = cl_addr_cur;
    if (confidence == 0) {
      stride = tmp_stride;
    } else if (stride != tmp_stride) {
      return metadata_in;
    }
    confidence++;
  }

  if (confidence == CONFIDENCE && stride != 0) {
    // active_lookahead = {champsim::address{cl_addr}, stride, PREFETCH_DEGREE};
    active_lookahead = {
        champsim::address{cl_addr + (PREFETCH_DISTANCE - 1) * stride}, stride,
        PREFETCH_DEGREE};
  }
  return metadata_in;
}

void ghb_stride_sa::adjust_degree() {
  if (intern_->sim_stats.pf_issued == 0)
    return;
  uint8_t accuracy = static_cast<uint8_t>((100 * intern_->sim_stats.pf_useful) /
                                          intern_->sim_stats.pf_issued);
  uint8_t bandwidth = static_cast<uint8_t>((get_dram_bw() * 100) / 16);
  if (bandwidth >= 75) {
    if (accuracy < 50)
      PREFETCH_DEGREE -= 2;
    else if (accuracy < 90)
      PREFETCH_DEGREE -= 1;
  } else if (bandwidth < 25) {
    if (accuracy >= 90)
      PREFETCH_DEGREE += 2;
    else if (accuracy >= 50)
      PREFETCH_DEGREE += 1;
  } else {
    if (accuracy >= 90)
      PREFETCH_DEGREE += 1;
    else if (accuracy < 50)
      PREFETCH_DEGREE -= 1;
  }
  PREFETCH_DEGREE = (PREFETCH_DEGREE < 1)   ? 1
                    : (PREFETCH_DEGREE > 6) ? 6
                                            : PREFETCH_DEGREE;
}

void ghb_stride_sa::prefetcher_cycle_operate() {
  if (cycle_cnt < 1000)
    cycle_cnt++;
  else {
    cycle_cnt = 0;
    adjust_degree();
  }

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
      // const bool mshr_under_light_load =
      //     intern_->get_mshr_occupancy_ratio() < 0.5;
      const bool mshr_under_light_load = true;
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

uint32_t ghb_stride_sa::prefetcher_cache_fill(champsim::address addr, long set,
                                              long way, uint8_t prefetch,
                                              champsim::address evicted_addr,
                                              uint32_t metadata_in) {
  return metadata_in;
}
