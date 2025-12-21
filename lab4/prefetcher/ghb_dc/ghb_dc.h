#ifndef GHB_DC_H
#define GHB_DC_H

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include "address.h"
#include "champsim.h"
#include "modules.h"
#include "msl/lru_table.h"

using idx_t = size_t;

struct index_table_entry {
  champsim::address ip{0};                          // IP
  champsim::block_number::difference_type delta{0}; // delta correlation
  idx_t ghb_idx{0};                                 // GHB pointer
};

struct ghb_entry {
  champsim::block_number::difference_type delta{0}; // delta correlation
  champsim::block_number cl_addr{0};                // delta correlation
  idx_t prev{0};
};

// Index table circular buffer
template <size_t N> struct it_cb {
  void push(const champsim::block_number::difference_type &delta,
            idx_t ghb_idx) {
    buf[head].delta = delta;
    buf[head].ghb_idx = ghb_idx;
    head = (head + 1) % N;

    if (size < N)
      size++;
  }

  index_table_entry buf[N];
  size_t head = 0;
  size_t size = 0;
};

// GHB circular buffer
template <size_t N> struct ghb_cb {
  void push(const champsim::block_number::difference_type &delta,
            const champsim::block_number cl_addr, idx_t prev) {
    buf[head].delta = delta;
    buf[head].cl_addr = cl_addr;
    buf[head].prev = prev;
    head = (head + 1) % N;

    if (size < N)
      size++;
  }

  ghb_entry buf[N];
  size_t head = 0;
  size_t size = 0;
};

struct ghb_dc : public champsim::modules::prefetcher {

  constexpr static std::size_t IT_SIZE = 256;
  constexpr static std::size_t IT_WAYS = 1;
  constexpr static std::size_t GHB_SIZE = 256;
  constexpr static int PREFETCH_WIDTH = 4;
  constexpr static int PREFETCH_DEPTH = 4;
  constexpr static int CONFIDENCE = 2;

  std::array<champsim::address, PREFETCH_DEPTH * PREFETCH_WIDTH>
      active_lookahead;
  size_t prefetch_cnt{0};

  ghb_cb<GHB_SIZE> ghb;
  it_cb<IT_SIZE> index_table; // FIFO

  std::array<champsim::address, PREFETCH_DEPTH * PREFETCH_WIDTH> prefetches;
  uint8_t prefetches_head{0};

public:
  using champsim::modules::prefetcher::prefetcher;

  uint32_t prefetcher_cache_operate(champsim::address addr,
                                    champsim::address ip, uint8_t cache_hit,
                                    bool useful_prefetch, access_type type,
                                    uint32_t metadata_in);
  uint32_t prefetcher_cache_fill(champsim::address addr, long set, long way,
                                 uint8_t prefetch,
                                 champsim::address evicted_addr,
                                 uint32_t metadata_in);
  void prefetcher_cycle_operate();
  void adjust_degree();
};

#endif
