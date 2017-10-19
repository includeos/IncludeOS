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
#include <common>
#include <util/fixed_list_alloc.hpp>
#include <list>

using seq_t = uint32_t;

namespace net {
namespace tcp {
namespace sack {

struct Block {
  Block(seq_t start, seq_t end)
    : start(start), end(end)
  {}

  Block() : whole{0}
  {}

  union {
    struct {
      seq_t start;
      seq_t end;
    };

    uint64_t whole = 0;
  };

  bool operator==(const Block& other) const noexcept
  { return start == other.start and end == other.end; }

  uint32_t size() const noexcept { return end - start; }
  bool empty () const noexcept { return whole == 0; }

  //int32_t precedes (const Block& other) const

  bool connects_start(const Block& other) const noexcept
  { return other.end == start; }

  bool connects_end(const Block& other) const noexcept
  { return other.start == end; }

  Block& operator=(uint64_t whole)
  { this->whole = whole; return *this; }

}__attribute__((packed));

// Print for Block
std::ostream& operator<<(std::ostream& out, const Block& b) {
  out << "[" << b.start << " => " << b.end << "]";
  return out;
}

using Entries = std::array<Block,3>;

struct Ack_result {
  Entries   entries;
  uint32_t  bytes;
};


template <typename List_impl>
class List {
public:

  Ack_result recv_out_of_order(seq_t seq, size_t len)
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
class Fixed_list {
public:
  static auto constexpr size = N;
  using List          = std::list<Block, Fixed_list_alloc<Block, N>>;
  using List_iterator = typename List::iterator;

  static_assert(N <= 32 && N > 0, "N wrong sized - optimized for small N");

  Ack_result recv_out_of_order(seq_t seq, uint32_t len)
  {
    Block inc{seq, seq+len};

    auto connected_end    = blocks.end();
    auto connected_start  = blocks.end();

    for(auto it = blocks.begin(); it != blocks.end(); it++)
    {
      Expects(not it->empty());

      if (it->connects_end(inc))
      {
        connected_end = it;
      }
      else if (it->connects_start(inc))
      {
        connected_start = it;
      }

      // if we connected to two nodes, no point in looking for more
      if (connected_start != blocks.end() and connected_end != blocks.end())
        break;
    }

    if (connected_end != blocks.end()) // Connectes to two blocks, e.g. fill a hole
    {
      connected_end->end = inc.end;

      // It also connects to a start
      if (connected_start != blocks.end())
      {
        connected_end->end = connected_start->end;
        blocks.erase(connected_start);
      }

      move_to_front(connected_end);
    }
    else if (connected_start != blocks.end()) // Connected only to an start
    {
      connected_start->start = inc.start;
      move_to_front(connected_start);
    }
    else // No connection - new entry
    {
      Expects(blocks.size() <= size);

      if(UNLIKELY(blocks.size() == size))
        return {recent_entries(), 0};

      blocks.push_front(inc);
    }

    return {recent_entries(), len};
  }

  Ack_result new_valid_ack(seq_t seq)
  {
    uint32_t bytes_freed = 0;

    for(auto it = blocks.begin(); it != blocks.end(); it++)
    {
      if (it->start == seq)
      {
        bytes_freed = it->size();
        blocks.erase(it);
        break;
      }
    };

    return {recent_entries(), bytes_freed};
  }

  void move_to_front(List_iterator it)
  {
    if(it != blocks.begin())
      blocks.splice(blocks.begin(), blocks, it);
  }

  Entries recent_entries() const
  {
    Entries ret;
    int i = 0;

    for(auto it = blocks.begin(); it != blocks.end() and i < ret.size(); it++)
      ret[i++] = *it;

    return ret;
  }

  List blocks;

};

} // < namespace sack
} // < namespace tcp
} // < namespace net
