// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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

#ifndef UTIL_ALLOC_BUDDY_HPP
#define UTIL_ALLOC_BUDDY_HPP

#include <common>
#include <sstream>
#include <array>
#include <util/bitops.hpp>
#include <util/units.hpp>
#include <common>
#include <stdlib.h>
#include <math.h>

#include <kprint>

//
// Tree node flags
//

namespace mem::buddy {
  enum class Flags : uint8_t {
    free = 0,
    taken = 1,
    left_used = 2,
    right_used = 4,
    left_full = 8,
    right_full = 16,
  };
}

namespace util {
  template<>
  struct enable_bitmask_ops<uint8_t> {
    using type = uint8_t;
    static constexpr bool enable = true;
  };

  template<>
  struct enable_bitmask_ops<mem::buddy::Flags> {
    using type = uint8_t;
    static constexpr bool enable = true;
  };
}

namespace mem::buddy {
  using namespace util::literals;
  using namespace util::bitops;

  using Node_t    = uint8_t;
  using Addr_t    = uintptr_t; // Use void* only at outermost api level
  using Size_t    = size_t;
  using Node_arr  = gsl::span<Node_t>;
  using Index_t   = Node_arr::index_type;

  /**
   * A buddy allocator over a fixed size pool
   **/
  template <bool Track_allocs = false>
  struct Alloc {
    static constexpr mem::buddy::Size_t min_size  = 4096;
    static constexpr mem::buddy::Size_t align = min_size;

    static constexpr Index_t node_count(mem::buddy::Size_t pool_size){
      return ((pool_size / min_size) * 2) - 1;
    }



    /** Total allocatable memory in an allocator created in a bufsize buffer **/
    static Size_t pool_size(Size_t bufsize){
      using namespace util;

      // Find closest power of two below bufsize
      auto pool_size_ = bits::keeplast(bufsize - 1);
      auto unalloc    = bufsize - pool_size_;
      auto node_size  = node_count(pool_size_) * sizeof(Node_t);
      auto overhead   = bits::roundto(min_size, sizeof(Alloc) + node_size);

      // If bufsize == overhead + pow2, and overhead was too small to fit alloc
      // Try the next power of two recursively
      if(unalloc < overhead)
        return pool_size(pool_size_);

      auto free      = bufsize - overhead;
      auto pool_size = bits::keeplast(free);
      return pool_size;
    }


    /** Required total bufsize to manage pool_size memory **/
    static Size_t required_size(Size_t pool_size) {
      using namespace util;
      return bits::roundto(min_size, pool_size)
        + bits::roundto(min_size, sizeof(Alloc) + node_count(pool_size) * sizeof(Node_t));
    }

    Index_t node_count() const noexcept {
      return node_count(pool_size_);
    }

    int leaf0() const noexcept {
      return node_count() / 2 - 1;
    }

    int tree_height() const noexcept {
      return util::bits::fls(node_count());
    }

    int tree_width() const noexcept {
      return node_count() / 2 + 1;
    }

    int max_chunksize() const noexcept {
      // Same as pool size.
      return min_size << (tree_height() - 1);
    }

    Size_t pool_size()
    { return pool_size_; }


    Size_t bytes_used() {
      return bytes_used_;
    }

    Addr_t highest_used() {
      return root().highest_used();
    }

    Size_t bytes_free(){
      return pool_size_ - bytes_used_;
    }

    Size_t bytes_free_r(){
      return pool_size_ - root().bytes_used();
    }

    Size_t bytes_used_r() {
      return root().bytes_used_r();
    }

    bool full() {
      return root().is_full();
    }

    bool empty() {
      return root().is_free();
    }

    Addr_t addr_begin() {
      return reinterpret_cast<Addr_t>(start_addr_);
    }

    Addr_t addr_end() {
      return start_addr_ + pool_size_;
    }

    bool in_range(void* a) {
      auto addr = reinterpret_cast<Addr_t>(a);
      return addr >= addr_begin() and addr < addr_end();
    }

    static_assert(util::bits::is_pow2(min_size), "Page size must be power of 2");


    Alloc(void* start, Size_t bufsize, Size_t pool_size)
      : nodes_{(Node_t*)start, node_count(pool_size)},
        start_addr_{util::bits::roundto(min_size, (Addr_t)start + node_count(pool_size))},
        pool_size_{pool_size}

    {
      using namespace util;
      Expects(bits::is_pow2(pool_size_));
      Expects(pool_size_ >= min_size);
      Expects(bits::is_aligned<min_size>(start_addr_));
      Ensures(pool_size_ + nodes_.size() * sizeof(Node_t) <= bufsize);
      // Initialize nodes
      memset(nodes_.data(), 0, nodes_.size() * sizeof(Node_arr::element_type));
    }

    static Alloc* create(void* addr, Size_t bufsize) {
      using namespace util;
      Size_t pool_size_ = pool_size(bufsize);
      Expects(bufsize >= required_size(pool_size_));

      // Placement new an allocator on addr, passing in the rest of memory
      auto* alloc_begin = (char*)addr + sizeof(Alloc);
      auto* alloc       = new (addr) Alloc(alloc_begin, bufsize, pool_size_);
      return alloc;
    }


    Size_t chunksize(Size_t wanted_sz) {
      auto sz = util::bits::next_pow2(wanted_sz);
      if (sz > max_chunksize())
        return 0;
      return std::max(sz, min_size);
    }

    enum class Track {
      get   = 0,
      inc   = 1,
      start = 2
    };

    struct Track_res {
      int last;
      int total;
      int min;
      int max;
      int allocs;
    };

    Track_res alloc_tracker(Track action = Track::get) {
      if constexpr (Track_allocs) {
          static Track_res tr {0,0,0,0,0};
          if (action == Track::start) {

            tr.allocs++;

            if (tr.last < tr.min or tr.min == 0)
              tr.min = tr.last;

            if (tr.last > tr.max)
              tr.max = tr.last;

            tr.total += tr.last;
            tr.last  = 0;

          } else if (action == Track::inc){
            tr.last++;
          }
          return tr;
        }
      return {};
    }

    void* allocate(Size_t size) {

      Expects(start_addr_);

      auto node = root();
      auto addr = start_addr_;

      auto sz = chunksize(size);
      if (not sz) return 0;

      // Start allocation tracker
      alloc_tracker(Track::start);

      auto res = node.allocate(sz);
      if (res) bytes_used_ += sz;
      return reinterpret_cast<void*>(res);
    }

    void deallocate(void* addr, Size_t size) {
      auto sz = size ? chunksize(size) : 0;
      auto res = root().deallocate((Addr_t)addr, sz);
      Expects(not size or res == sz);
      bytes_used_ -= res;
    }

    void free(void* addr) {
      deallocate(addr, 0);
    }


    //private:
    class Node_view {
    public:

      Node_view(int index, Alloc* alloc)
        : i_{index}, alloc_{alloc},
          my_size_{compute_size()}, my_addr_{compute_addr()}
      {}

      Node_view() = default;

      Node_view left() {
        if (is_leaf()) {
          return Node_view();
        }
        return Node_view(i_ * 2 + 1, alloc_);
      }

      Node_view right() {
        if (is_leaf()) {
          return Node_view();
        }
        return Node_view(i_ * 2 + 2, alloc_);
      }

      Addr_t addr()
      { return my_addr_; }

      Size_t size()
      { return my_size_; }

      bool is_leaf() {
        return i_ >= alloc_->nodes_.size() / 2;
      }

      bool is_taken() {
        return data() & Flags::taken;
      }

      bool is_free() {
        return data() == 0;
      }

      bool is_full() {
        auto fl = data();
        return fl & Flags::taken or
          (fl & Flags::left_full and fl & Flags::right_full);
      }

      bool in_range(Addr_t a) {
        return a >= my_addr_ and a < my_addr_ + my_size_;
      }

      void set_flag(Flags f){
        data() |= f;
      }

      void clear_flag(Flags f){
        data() &= ~f;
      }

      bool is_parent() {
        return i_ <= alloc_->leaf0();
      }


      bool is_full_r() {
        if (is_taken())
          return true;
        return is_parent() and left().is_full_r()
          and is_parent() and right().is_full_r();
      }

      bool is_free_r() {
        auto lfree = not is_parent() or left().is_free_r();
        auto rfree = not is_parent() or right().is_free_r();
        return not is_taken() and lfree and rfree;
      }


      int height() {
        return (util::bits::fls(i_ + 1));
      }

      Size_t compute_size() {
        return min_size << (alloc_->tree_height() - height());
      }

      Addr_t compute_addr() {
        auto first_lvl_idx = 1 << (height() - 1);
        auto sz_offs = i_ - first_lvl_idx + 1;
        return alloc_->start_addr_ + (sz_offs * my_size_);
      }

      operator bool() {
        return alloc_->start_addr_ != 0 and alloc_ != nullptr;
      }

      Node_t& data() {
        return alloc_->nodes_.at(i_);
      }

      Addr_t allocate_self() {
        Expects(not (data() & Flags::taken));
        set_flag(Flags::taken);
        return my_addr_;
      }

      Addr_t allocate(Size_t sz) {
        using namespace util;

        // Track allocation steps
        alloc_->alloc_tracker(Track::inc);

        Expects(sz <= my_size_);
        Expects(! is_taken());

        if (is_taken()) {
          return 0;
        }

        // Allocate self if possible
        if (sz == my_size_) {
          if (is_free()) {
            return allocate_self();
          }
          return 0;
        }

        if (not is_parent())
          return 0;

        // Try left allocation
        auto lhs = left();
        if (not lhs.is_full()) {

          auto addr_l = lhs.allocate(sz);

          if (lhs.is_full())
            set_flag(Flags::left_full);

          if (addr_l) {
            set_flag(Flags::left_used);
            return addr_l;
          }
        }

        // Try right allocation
        auto rhs = right();
        if (not rhs.is_full()) {

          auto addr_r = rhs.allocate(sz);

          if (rhs.is_full())
            set_flag(Flags::right_full);

          if (addr_r != 0) {
            set_flag(Flags::right_used);
            return addr_r;
          }
        }

        return 0;
      }

      // Left / right deallocation
      // TODO: there must be a pattern to make this DRY in a nice way
      //       also for the left / right allocation steps above.

      Size_t dealloc_left(Addr_t ptr, Size_t sz) {
        auto lhs = left();
        auto res = lhs.deallocate(ptr, sz);
        if (not lhs.is_full()) {
          clear_flag(Flags::left_full);
        }
        if (lhs.is_free()) {
          clear_flag(Flags::left_used);
        }
        return res;
      }

      Size_t dealloc_right(Addr_t ptr, Size_t sz) {
        auto rhs = right();
        auto res = rhs.deallocate(ptr, sz);
        if (not rhs.is_full()){
          clear_flag(Flags::right_full);
        }
        if (rhs.is_free()) {
          clear_flag(Flags::right_used);
        }
        return res;
      }

      Size_t dealloc_self(){
        data() = 0;
        return my_size_;
      }

      bool is_left(Addr_t ptr){
        return ptr < my_addr_ + my_size_ / 2;
      }

      bool is_right(Addr_t ptr) {
        return ptr >= my_addr_ + my_size_ / 2;
      }

      Addr_t deallocate(Addr_t ptr, Size_t sz = 0) {
        using namespace util;

        #ifdef DEBUG_UNIT
        printf("Node %i DE-allocating 0x%zx size %zi \n", i_, ptr, sz);
        #endif

        Expects(not (is_taken() and (data() & Flags::left_used
                                     or data() & Flags::right_used)));


        // Don't try if ptr is outside range
        if (not in_range(ptr)) {
          return 0;
        }

        if (is_taken()) {
          if (ptr != my_addr_)
            return 0;

          if (sz)
            Expects(my_size_ == sz);

          return dealloc_self();
        }

        if (not is_parent()) {
          return 0;
        }

        if (is_left(ptr)) {
          return dealloc_left(ptr, sz);
        }

        if (is_right(ptr)) {
          return dealloc_right(ptr, sz);
        }

        return 0;
      }

      Size_t bytes_used() {
        if (is_taken())
          return my_size_;

        if (is_parent())
          return left().bytes_used() + right().bytes_used();

        return 0;
      }

      Addr_t highest_used() {
        if (is_taken() or not is_parent())
          return my_addr_ + my_size_;

        auto rhs = right().highest_used();
        if (rhs > my_addr_ + my_size_) return rhs;
        return left().highest_used();
      }

      std::string to_string(){
        std::stringstream out;
        out << std::hex
          // << addr()<< " | "
            <<  (int)data() //is_full_r() //
            << std::dec;
        return out.str();
      }

    private:
      int i_ = 0;
      Alloc* alloc_ = nullptr;
      Size_t my_size_ = 0;
      Addr_t my_addr_ = 0;
    };

    Node_view root()
    { return Node_view(0, this); }

    std::string summary() {
      std::stringstream out;
      std::string dashes(80, '-');
      out << dashes << "\n";
      out << "Bytes used: " << util::Byte_r(bytes_used())
          << " Bytes free: " << util::Byte_r(bytes_free())
          << " H: " << std::dec << tree_height()
          << " W: " << tree_width()
          << " Alloc.size: " << util::Byte_r(sizeof(*this)) << "\n"
          << "Address pool: 0x" << std::hex
          << root().addr() << " - " << root().addr() + pool_size_
          << std::dec << " ( " << util::Byte_r(pool_size()) <<" ) \n";
      auto track = alloc_tracker();
      out << "Allocations:  " << track.allocs
          << " Steps: Last: " << track.last
          << " Min:  " << track.min
          << " Max:  " << track.max
          << " Total: " << track.total
          << " Avg: " << track.total / track.allocs
          << "\n";
      out << dashes << "\n";
      return out.str();
    }
    std::string draw_tree(bool index = false) {
      auto node_pr_w = index ? 6 : 4;
      auto line_w = tree_width() * node_pr_w;

      std::stringstream out{summary()};
      std::string spaces (line_w, ' ');
      std::string dashes (line_w, '-');

      int i = 0;

      for (int h = 0; h < tree_height(); h++) {
        auto cnt = 1 << h;
        auto prw = cnt * node_pr_w;
        int loff  = (spaces.size() / 2) - (prw / 2);
        // Line offset
        auto nsize = min_size << (tree_height() - h - 1);
        printf("%s\t", util::Byte_r(nsize).to_string().c_str());
        printf("%.*s",  loff, spaces.c_str());
        out.flush();
        while (cnt--) {
          printf("[%s] ",  Node_view(i, this).to_string().c_str());
          i++;
        }
        printf("\n");
      }
      out << dashes << "\n";
      return out.str();
    }

    //using Tracker = typename std::enable_if<Track_allocs, Track_res>::type;
    Track_res tr {0,0,0,0,0}; // If tracking enabled

    Node_arr nodes_;
    const uintptr_t start_addr_ = 0;
    const Size_t pool_size_ = min_size;
    Size_t bytes_used_ = 0;
  };

}

#endif
