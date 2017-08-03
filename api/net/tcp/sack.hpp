// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstdint>
#include <array>
#include <iostream>

using seq_t = uint32_t;

namespace net {
namespace tcp {
namespace sack {

union Block {
  Block(seq_t start, seq_t end)
    : start(start), end(end)
  {}

  Block() : whole{0}
  {}

  struct {
    seq_t start;
    seq_t end;
  };

  uint64_t whole = 0;

  bool operator==(const Block& other) const noexcept
  { return start == other.start and end == other.end; }

  uint32_t size() const noexcept { return end - start; }
  bool empty () const noexcept { return whole == 0; }

  //int32_t precedes (const Block& other) const

  bool connect_start(const Block& other) const noexcept
  { return other.end == start; }

  bool connect_end(const Block& other) const noexcept
  { return other.start == end; }

  Block& operator=(uint64_t whole)
  { this->whole = whole; return *this; }

}__attribute__((packed));

// Print for Block
std::ostream& operator<<(std::ostream& out, const Block& b) {
  out << "Start: " << b.start << " End: " << b.end;
  return out;
}

using Entries = std::array<Block,3>;

struct Ack_result {
  Entries   entries;
  uint32_t  bytes_freed;
};


template <typename List_impl>
class List {
public:

  Entries recv_out_of_order(seq_t seq, size_t len)
  {
    return impl.recv_out_of_order(seq, len);
  }

  Ack_result new_valid_ack(seq_t end)
  {
    return impl.new_valid_ack(end);
  }

  List_impl impl;

};

template <int N = 3>
class Array_list {
public:
  static_assert(N <= 32 && N > 0, "N wrong sized - optimized for small N");

  Entries recv_out_of_order(seq_t seq, uint32_t len)
  {
    Block inc{seq, seq+len};

    int connected_end = -1;
    int connected_start = -1;
    int idx = (latest[0] >= 0) ? latest[0] : 0;
    int i = idx;
    int free = -1;
    do {
      if (blocks[i].empty()) {
        free = i;
        continue;
      }

      if (blocks[i].connect_end(inc))
      {
        std::cout << "Block [ " << inc << " ] "
          << "connects to end of [ " << blocks[i] << " ]\n";

        connected_end = i;
      }
      else if (blocks[i].connect_start(inc))
      {
        std::cout << "Block [ " << inc << " ] "
          << "connects to start of [ " << blocks[i] << " ]\n";

        connected_start = i;
      }

      if (connected_start >= 0 and connected_end >= 0)
        break;

      // increment i, continue as long as we havent wrapped around (to idx)
    } while((i = (i+1) % N) != idx);

    int latest_idx = -1;
    // Connectes to two blocks, e.g. fill a hole
    if (connected_end >= 0) {

      auto& update = blocks[connected_end];
      std::cout << "1: " << update << "\n";
      update.end = inc.end;
      std::cout << "2: " << update << "\n";

      // It also connects to a start
      if(connected_start >= 0)
      {
        update.end = blocks[connected_start].end;
        std::cout << "3: " << update << "\n";
        blocks[connected_start] = 0;
      }

      latest_idx = connected_end;
      // Connected only to an end
    } else if (connected_start >= 0) {
      blocks[connected_start].start = inc.start;

      latest_idx = connected_start;
      // No connection - new entry
    } else {
      if(free >= 0) {
        blocks[free] = inc;
        latest_idx = free;
      }
      else
        std::cout << "Cannot add block at idx: " << idx << "\n";
    }

    update_latest(latest_idx);

    auto entries = recent_entries();

    return entries;
  }

  Ack_result new_valid_ack(seq_t seq)
  {
    int idx = (latest[0] >= 0) ? latest[0] : 0;
    int i = idx;
    uint32_t bytes_freed = 0;
    do {
      if (blocks[i].empty())
        continue;

      if (blocks[i].start == seq) {
        bytes_freed = blocks[i].size();
        blocks[i] = 0;
        clear_latest(i);
        break;
      }

      // increment i, continue as long as we havent wrapped around (to idx)
    } while((i = (i+ 1) % N) != idx);

    return {recent_entries(), bytes_freed};
  }

  void update_latest(int idx)
  {
    if(idx == latest[0])
      return;

    if(idx != latest[1])
      latest[2] = latest[1];

    latest[1] = latest[0];
    latest[0] = idx;
  }

  void clear_latest(int idx)
  {

  }

  Entries recent_entries() const
  {
    return {{
      (latest[0] >= 0) ? blocks[latest[0]] : Block(),
      (latest[1] >= 0) ? blocks[latest[1]] : Block(),
      (latest[2] >= 0) ? blocks[latest[2]] : Block()
    }};
  }

  std::array<Block, N> blocks;
  std::array<int, 3> latest = {{-1,-1,-1}};

};

} // < namespace sack
} // < namespace tcp
} // < namespace net
