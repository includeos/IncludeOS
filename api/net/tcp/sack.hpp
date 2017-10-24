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

  void swap_endian() noexcept
  {
    start = htonl(start);
    end   = htonl(end);
  }

  bool operator==(const Block& other) const noexcept
  { return start == other.start and end == other.end; }

  uint32_t size() const noexcept { return end - start; }
  bool empty () const noexcept { return whole == 0; }

  bool contains(const seq_t seq) const noexcept
  {
    return static_cast<int32_t>(seq - start) >= 0 and
           static_cast<int32_t>(seq - end) <= 0;
  }

  bool precedes(const seq_t seq) const noexcept
  { return static_cast<int32_t>(end - seq) <= 0; }

  bool precedes(const seq_t seq, const Block& other) const noexcept
  { return (start - seq) < (other.start - seq); }

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

using Entries = Fixed_vector<Block,3>;

struct Ack_result {
  Entries   entries;
  uint32_t  bytes;
};

template <typename List_impl>
class List {
public:
  List() = default;

  List(List_impl&& args)
    : impl{std::forward(args)}
  {}

  Ack_result recv_out_of_order(const seq_t seq, const size_t len)
  { return impl.recv_out_of_order({seq, static_cast<seq_t>(seq+len)}); }

  Ack_result new_valid_ack(const seq_t end)
  { return impl.new_valid_ack(end); }

  List_impl impl;
};

template <typename Iterator>
struct Connect_result {
  Iterator end;
  Iterator start;
};

template <typename Iterator, typename Connectable>
Connect_result<Iterator>
connects_to(Iterator first, Iterator last, const Connectable& value)
{
  Connect_result<Iterator> connected{last, last};
  for (; first != last; ++first)
  {
    if (first->connects_end(value))
    {
      connected.end = first;
    }
    else if (first->connects_start(value))
    {
      connected.start = first;
    }

    // if we connected to two nodes, no point in looking for more
    if (connected.start != last and connected.end != last)
      break;
  }
  return connected;
}

template <int N = 3>
class Fixed_list {
public:
  static auto constexpr size = N;
  using List          = std::list<Block, Fixed_list_alloc<Block, N>>;
  using List_iterator = typename List::iterator;

  static_assert(N <= 32 && N > 0, "N wrong sized - optimized for small N");

  Ack_result recv_out_of_order(Block blk)
  {
    auto connected = connects_to(blocks.begin(), blocks.end(), blk);

    if (connected.end != blocks.end()) // Connectes to an end
    {
      connected.end->end = blk.end;

      // It also connects to a start, e.g. fills a hole
      if (connected.start != blocks.end())
      {
        connected.end->end = connected.start->end;
        blocks.erase(connected.start);
      }

      move_to_front(connected.end);
    }
    else if (connected.start != blocks.end()) // Connected only to an start
    {
      connected.start->start = blk.start;
      move_to_front(connected.start);
    }
    else // No connection - new entry
    {
      Expects(blocks.size() <= size);

      if(UNLIKELY(blocks.size() == size))
        return {recent_entries(), 0};

      blocks.push_front(blk);
    }

    return {recent_entries(), blk.size()};
  }

  Ack_result new_valid_ack(const seq_t seq)
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
    }

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

    for(auto it = blocks.begin(); it != blocks.end() and ret.size() < ret.capacity(); it++)
      ret.push_back(*it);

    return ret;
  }

  List blocks;

};

// SCOREBOARD stuff

template <typename Scoreboard_impl>
class Scoreboard {
public:
  void recv_sack(const seq_t current, Block blk)
  { return impl.recv_sack(current, blk); }

  void recv_sack(const seq_t current, seq_t start, seq_t end)
  { return recv_sack(current, {start, end}); }

  void new_valid_ack(const seq_t seq)
  { return impl.new_valid_ack(seq); }

  void clear()
  { impl.clear(); }

  Scoreboard_impl impl;
};

template <int N = 3>
class Scoreboard_list {
public:
  static auto constexpr size = N;
  using List          = std::list<Block, Fixed_list_alloc<Block, N>>;
  using List_iterator = typename List::iterator;

  void recv_sack(const seq_t current, Block blk)
  {
    auto connected = connects_to(blocks.begin(), blocks.end(), blk);

    if (connected.end != blocks.end()) // Connectes to an end
    {
      connected.end->end = blk.end;

      // It also connects to a start, e.g. fills a hole
      if (connected.start != blocks.end())
      {
        connected.end->end = connected.start->end;
        blocks.erase(connected.start);
      }
    }
    else if (connected.start != blocks.end()) // Connected only to an start
    {
      connected.start->start = blk.start;
    }
    else // No connection - new entry
    {
      insert(current, blk);
    }
  }

  void new_valid_ack(const seq_t seq)
  {
    for(auto it = blocks.begin(); it != blocks.end(); )
    {
      auto tmp = it++;

      if (tmp->precedes(seq))
        blocks.erase(tmp);
    }
  }

  void clear()
  { blocks.clear(); }

  void insert(const seq_t current, Block blk)
  {
    Expects(blocks.size() <= size);

    if(UNLIKELY(blocks.size() == size))
      return;

    auto it = std::find_if(blocks.begin(), blocks.end(),
      [&](const auto& block) {
        return blk.precedes(current, block);
    });

    blocks.insert(it, blk);
  }

  List blocks;
};

} // < namespace sack
} // < namespace tcp
} // < namespace net
