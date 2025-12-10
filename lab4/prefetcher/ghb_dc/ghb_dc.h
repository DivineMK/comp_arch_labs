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
  champsim::address ip{0}; // the IP we're tracking
  idx_t ghb_idx{0};
  auto index() const {
    using namespace champsim::data::data_literals;
    return ip.slice_upper<2_b>();
  }
  auto tag() const {
    using namespace champsim::data::data_literals;
    return ip.slice_upper<2_b>();
  }
};

struct ghb_entry {
  champsim::block_number::difference_type delta{0}; // the cache line
  idx_t prev_access_id{0};
};

// Index table circular buffer
template <size_t N> struct it_cb {
  void push(const champsim::address &ip, idx_t ghb_idx) {
    buf[head].ip = ip;
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
            idx_t prev_access_id) {
    buf[head].delta = delta;
    buf[head].prev_access_id = prev_access_id;
    head = (head + 1) % N;

    if (size < N)
      size++;
  }

  ghb_entry buf[N];
  size_t head = 0;
  size_t size = 0;
};

struct ghb_dc : public champsim::modules::prefetcher {

  struct lookahead_entry {
    champsim::address address{0};
    champsim::address::difference_type stride{0};
  };

  constexpr static std::size_t IT_SIZE = 256;
  constexpr static std::size_t IT_WAYS = 1;
  constexpr static std::size_t GHB_SIZE = 256;
  constexpr static int PREFETCH_WIDTH = 4;
  constexpr static int PREFETCH_DEPTH = 4;
  constexpr static int CONFIDENCE = 2;

  std::array<lookahead_entry, PREFETCH_DEPTH * PREFETCH_WIDTH> active_lookahead;

  ghb_cb<GHB_SIZE> ghb;
  // it_cb<IT_SIZE> index_table; // FIFO
  champsim::msl::lru_table<index_table_entry> index_table{IT_SIZE,
                                                          IT_WAYS}; // LRU

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
