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

  bool operator==(const Block& other) const
  { return start == other.start and end == other.end; }

  uint32_t size() const { return end - start; }
  bool empty () const { return whole == 0; }

  //int32_t precedes (const Block& other) const

  bool connect_start(const Block& other) const {
    return other.end == start;
  }

  bool connect_end(const Block& other) const {
    return other.start == end;
  }

}__attribute__((packed));


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

    inc.start;
    inc.end;

    printf("idx: %i \n", idx);

    int connected_end = -1;
    int connected_start = -1;
    int i = idx;
    do {
      if (blocks[i].empty())
        continue;

      if (blocks[i].connect_end(inc))
      {
        std::cout << "Block [ " << inc << " ] "
          << "connects end to [ " << blocks[i] << " ]\n";

        connected_end = i;
      }
      else if (blocks[i].connect_start(inc))
      {
        std::cout << "Block [ " << inc << " ] "
          << "connects start to [ " << blocks[i] << " ]\n";

        connected_start = i;
      }

      if (connected_start >= 0 and connected_end >= 0)
        break;

      // increment i, continue as long as we havent wrapped around (to idx)
    } while((i = (i+ 1) % N) != idx);

    // Connectes to two blocks, e.g. fill a hole
    if (connected_end >= 0 and connected_start >= 0) {


      // Connected only to an end
    } else if (connected_end >= 0) {
      blocks[connected_end].end = inc.end;
      idx = connected_end;
      // Connected only to a begin
    } else if (connected_start >= 0) {
      blocks[connected_start].start = inc.start;
      idx = connected_start;
      // No connection - new entry
    } else {
      if(blocks[idx].empty())
        blocks[idx] = inc;
      else
        std::cout << "Cannot add block at idx: " << idx << "\n";
    }

    auto entries = recent_entries();

    // not correct
    idx++;
    if(idx == N) idx = 0;

    return entries;
  }

  Ack_result new_valid_ack(seq_t seq)
  {
    Entries list{};
    return {list, 0};
  }

  Entries recent_entries() const
  {
    // not correct
    return {{blocks[idx], blocks[idx-1 % N], blocks[idx-2 % N]}};
  }

  std::array<Block, N> blocks;
  int idx = 0;

};

} // < namespace sack
} // < namespace tcp
} // < namespace net
