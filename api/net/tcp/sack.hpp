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

  Block* older = nullptr;
  Block* newer = nullptr;

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


  void free() {
    detach();
    whole = 0;
    older = nullptr;
    newer = nullptr;
  }

  void detach() {
    if (newer)
      newer->older = older;
    if (older)
      older->newer = newer;

    older = nullptr;
    newer = nullptr;
  }


}__attribute__((packed));

// Print for Block
std::ostream& operator<<(std::ostream& out, const Block& b) {
  out << "[" << b.start << " => " << b.end << "]";
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

    Block* connected_end = nullptr;
    Block* connected_start = nullptr;
    Block* current = latest_;

    while (current) {

      Expects(not current->empty());

      if (current->connect_end(inc))
      {
        connected_end = current;
      }
      else if (current->connect_start(inc))
      {
        connected_start = current;
      }

      if (connected_start and connected_end)
        break;

      current = current->older;

    }

    Block* update = nullptr;

    // Connectes to two blocks, e.g. fill a hole
    if (connected_end) {

      update = connected_end;
      update->detach();
      update->end = inc.end;

      // It also connects to a start
      if(connected_start)
      {
        update->end = connected_start->end;
        connected_start->free();
      }

      // Connected only to an end
    } else if (connected_start) {
      update = connected_start;
      update->start = inc.start;
      update->detach();
      // No connection - new entry
    } else {
      update = get_free();
      if (not update) {
        // TODO: return older sack list
        printf("No free blocks left\n");
      } else {
        *update = inc;
      }
    }

    update_latest(update);
    return recent_entries();
  }

  Ack_result new_valid_ack(seq_t seq)
  {
    Block* current = latest_;
    uint32_t bytes_freed = 0;

    while (current) {

      if (current->start == seq) {
        bytes_freed = current->size();

        if (latest_ == current)
          latest_ = current->older;
        current->free();

        break;
      }

      current = current->older;

    };

    return {recent_entries(), bytes_freed};
  }

  void update_latest(Block* blk)
  {
    Expects(blk);
    Expects(!blk->empty());

    if (blk == latest_)
      return;

    blk->older = latest_;
    blk->newer = nullptr;

    if (latest_)
      latest_->newer = blk;

    latest_ = blk;
    Ensures(latest_);
    Ensures(latest_ != latest_->newer);
    Ensures(latest_ != latest_->older);
  }


  Block* get_free() {

    // TODO: Optimize.
    for (auto& block : blocks)
      if (block.empty())
        return &block;

    return nullptr;
  }

  Entries recent_entries() const
  {

    Entries ret;
    int i = 0;
    Block* current = latest_;

    while (current and i < ret.size()) {
      ret[i++] = *current;
      current = current->older;
    }

    return ret;
  }

  std::array<Block, N> blocks;

  Block* latest_ = nullptr;

};

} // < namespace sack
} // < namespace tcp
} // < namespace net
