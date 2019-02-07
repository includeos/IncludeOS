
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
#include <pmr>
#include <stdlib.h>
#include <math.h>

#include <util/bitops.hpp>
#include <util/units.hpp>

//
// Tree node flags
//

namespace os::mem::buddy {
  enum class Flags : uint8_t {
    free = 0,
    taken = 1,
    left_used = 2,
    right_used = 4,
    left_full = 8,
    right_full = 16
  };
}

namespace util {
  template<>
  struct enable_bitmask_ops<uint8_t> {
    using type = uint8_t;
    static constexpr bool enable = true;
  };

  template<>
  struct enable_bitmask_ops<os::mem::buddy::Flags> {
    using type = uint8_t;
    static constexpr bool enable = true;
  };
}

namespace os::mem::buddy {
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
  struct Alloc : public std::pmr::memory_resource {
    static constexpr mem::buddy::Size_t min_size  = 4096;
    static constexpr mem::buddy::Size_t align = min_size;

    static constexpr Index_t node_count(mem::buddy::Size_t pool_size){
      return ((pool_size / min_size) * 2) - 1;
    }

    static constexpr Size_t overhead(Size_t pool_size) {
      using namespace util;
      auto nodes_size  = node_count(pool_size) * sizeof(Node_t);
      auto overhead_   = bits::roundto(min_size, sizeof(Alloc) + nodes_size);
      return overhead_;
    }

    /**
     * Indicate if the allocator should manage a power of 2 larger than- or
     * smaller than available memory.
     **/
    enum class Policy {
      underbook, overbook
    };

    /**
     * Total allocatable memory in an allocator created in a bufsize buffer.
     * If the policy is overbook, we compute the next power of two above bufsize,
     * otherwise the next power of two below.
     **/
    template <Policy P>
    static Size_t pool_size(Size_t bufsize){
      using namespace util;

      auto pool_sz = 0;
      // Find closest usable power of two depending on policy
      if constexpr (P == Policy::overbook) {
          auto pow2 = bits::next_pow2(bufsize);

          // On 32 bit we might easily overflow
          if (pow2 > bufsize)
            return pow2;

      }

      pool_sz = bits::keeplast(bufsize - 1); // -1 starts the recursion

      auto unalloc   = bufsize - pool_sz;
      auto overhead  = Alloc::overhead(pool_sz);

      // If bufsize == overhead + pow2, and overhead was too small to fit alloc
      // Try the next power of two recursively
      if(unalloc < overhead)
        return Alloc::pool_size<P>(pool_sz);

      auto free = bufsize - overhead;
      return bits::keeplast(free);
    }


   /**
    * Maximum required bufsize to manage pool_size memory,
    * assuming the whole pool will be available in memory.
    **/
    static Size_t max_bufsize(Size_t pool_size) {
      using namespace util;
      return bits::roundto(min_size, pool_size)
        + overhead(pool_size);
    }

    /** Minimum total bufsize to manage pool_size memory **/
    template <Policy P>
    static Size_t min_bufsize(Size_t pool_size) {
      using namespace util;

      if constexpr (P == Policy::overbook) {
          return overhead(pool_size);
      }

      return max_bufsize(pool_size);
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

    Size_t max_chunksize() const noexcept {
      // Same as pool size.
      return min_size << (tree_height() - 1);
    }

    /** Theoretically allocatable capacity */
    Size_t pool_size() const noexcept {
      return pool_size_;
    }

    /** Total allocatable bytes, including allocated and free */
    Size_t capacity() const noexcept {
      return addr_limit_ - start_addr_;
    }

    /** Bytes unavailable in an overbooking allocator */
    Size_t bytes_unavailable() const noexcept {
      return pool_size_ - capacity();
    }

    Size_t bytes_used() const noexcept {
      return bytes_used_;
    }

    Addr_t highest_used() const noexcept {
      return std::min(root().highest_used_r(), start_addr_ + capacity());
    }

    Size_t bytes_free() const noexcept {
      return pool_size_ - bytes_used_ - bytes_unavailable();
    }

    Size_t bytes_free_r() const noexcept {
      return pool_size_ - root().bytes_used_r();
    }

    Size_t bytes_used_r() const noexcept {
      return root().bytes_used_r();
    }

    bool full() {
      return root().is_full();
    }

    bool overbooked() {
      return overbooked_;
    }

    bool empty() {
      return bytes_used() == 0;
    }

    Addr_t addr_begin() const noexcept {
      return reinterpret_cast<Addr_t>(start_addr_);
    }

    Addr_t addr_end() const noexcept {
      return start_addr_ + pool_size_;
    }

    bool in_range(void* a) const noexcept {
      auto addr = reinterpret_cast<Addr_t>(a);
      return addr >= addr_begin() and addr < addr_end();
    }

    static_assert(util::bits::is_pow2(min_size), "Page size must be power of 2");


    Alloc(void* start, Size_t bufsize, Size_t pool_size)
      : nodes_{(Node_t*)start, node_count(pool_size)},
        start_addr_{util::bits::roundto(min_size, (Addr_t)start + node_count(pool_size))}, addr_limit_{reinterpret_cast<uintptr_t>(start) + bufsize},
        pool_size_{pool_size}

    {
      using namespace util;
      Expects(bits::is_pow2(pool_size_));
      Expects(pool_size_ >= min_size);
      Expects(bits::is_aligned<min_size>(start_addr_));
      Ensures(bufsize >= overhead(pool_size));
      // Initialize nodes
      memset(nodes_.data(), 0, nodes_.size() * sizeof(Node_arr::element_type));
    }

    /**
     * Create an allocator
     **/
    template <Policy P = Policy::overbook>
    static Alloc* create(void* addr, Size_t bufsize) {
      using namespace util;
      Size_t pool_size_ = pool_size<P>(bufsize);
      Expects(pool_size_ >= min_size);
      Expects(bufsize >= min_bufsize<P>(pool_size_));
      // Placement new an allocator on addr, passing in the rest of memory
      auto* alloc_begin = (char*)addr + sizeof(Alloc);
      auto* alloc       = new (addr) Alloc(alloc_begin,
                                           bufsize - sizeof(Alloc),
                                           pool_size_);
      return alloc;
    }


    Size_t chunksize(Size_t wanted_sz) const noexcept {
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

    Track_res alloc_tracker(Track action = Track::get) const noexcept {
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

    void* allocate(Size_t size) noexcept {

      Expects(start_addr_);

      auto node = root();

      auto sz = chunksize(size);
      if (not sz) return 0;

      // Start allocation tracker
      alloc_tracker(Track::start);

      auto res = node.allocate(sz);

      // For overbooking allocator, allow unusable memory to gradually become
      // marked as allocated, without actually handing it out.
      if (UNLIKELY(res + size > addr_limit_)) {
        overbooked_ = true;
        return 0;
      }

      if (res) bytes_used_ += sz;
      return reinterpret_cast<void*>(res);
    }

    void deallocate(void* addr, Size_t size) {
      auto sz = size ? chunksize(size) : 0;
      Expects(reinterpret_cast<uintptr_t>(addr) + size < addr_limit_);
      auto res = root().deallocate((Addr_t)addr, sz);
      Expects(not size or res == sz);
      bytes_used_ -= res;
    }

    void* do_allocate(std::size_t bytes, std::size_t alignment)  override {
      using namespace util;
      auto aligned_size = bits::roundto(alignment, bytes);
      return allocate(aligned_size);
    }

    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
      using namespace util;
      deallocate(p, bits::roundto(alignment, bytes));
    }

    bool do_is_equal(const memory_resource& other) const noexcept override {
      return &other == this;
    }

    void free(void* addr) {
      deallocate(addr, 0);
    }


    //private:
    class Node_view {
    public:

      Node_view(int index, const Alloc* alloc)
        : i_{index}, alloc_{alloc},
          my_size_{compute_size()}, my_addr_{compute_addr()}
      {}

      Node_view() = default;

      Node_view empty() const noexcept {
        return Node_view();
      }

      Node_view left() const noexcept {
        if (is_leaf()) {
          return empty();
        }
        return Node_view(i_ * 2 + 1, alloc_);
      }

      Node_view right() const noexcept {
        if (is_leaf()) {
          return empty();
        }
        return Node_view(i_ * 2 + 2, alloc_);
      }

      Addr_t addr() const noexcept {
        return my_addr_;
      }

      Size_t size() const noexcept {
        return my_size_;
      }

      bool is_leaf() const noexcept {
        return i_ >= alloc_->nodes_.size() / 2;
      }

      bool is_taken() const noexcept {
        return data() & Flags::taken;
      }

      bool is_free() const noexcept {
        return data() == 0;
      }

      bool is_full() const noexcept {
        auto fl = data();
        return fl & Flags::taken or
          (fl & Flags::left_full and fl & Flags::right_full);
      }

      bool in_range(Addr_t a) const noexcept {
        return a >= my_addr_ and a < my_addr_ + my_size_;
      }

      void set_flag(Flags f) {
        data() |= f;
      }

      void clear_flag(Flags f) {
        data() &= ~f;
      }

      bool is_parent() const noexcept {
        return i_ <= alloc_->leaf0();
      }


      bool is_full_r() const noexcept {
        if (is_taken())
          return true;
        return is_parent() and left().is_full_r()
          and is_parent() and right().is_full_r();
      }

      bool is_free_r() const noexcept {
        auto lfree = not is_parent() or left().is_free_r();
        auto rfree = not is_parent() or right().is_free_r();
        return not is_taken() and lfree and rfree;
      }


      int height() const noexcept {
        return (util::bits::fls(i_ + 1));
      }

      Size_t compute_size() const noexcept {
        return min_size << (alloc_->tree_height() - height());
      }

      Addr_t compute_addr() const noexcept {
        auto first_lvl_idx = 1 << (height() - 1);
        auto sz_offs = i_ - first_lvl_idx + 1;
        return alloc_->start_addr_ + (sz_offs * my_size_);
      }

      operator bool() const noexcept {
        return alloc_->start_addr_ != 0 and alloc_ != nullptr;
      }

      Node_t& data() {
        return alloc_->nodes_.at(i_);
      }

      const Node_t& data() const noexcept {
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

      Size_t dealloc_self() {
        data() = 0;
        return my_size_;
      }

      bool is_left(Addr_t ptr) const noexcept {
        return ptr < my_addr_ + my_size_ / 2;
      }

      bool is_right(Addr_t ptr) const noexcept {
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

      Size_t bytes_used_r() const noexcept {
        if (is_taken())
          return my_size_;

        if (is_parent())
          return left().bytes_used_r() + right().bytes_used_r();

        return 0;
      }

      Addr_t highest_used_r() const noexcept {
        if (is_free()) {
          return my_addr_;
        }

        if (is_taken() or not is_parent()) {
          return my_addr_ + my_size_;
        }

        auto right_ = right();
        if (not right_.is_free()) {
          return right_.highest_used_r();
        }

        auto left_ = left();
        Expects(not left_.is_free());
        return left_.highest_used_r();
      }

      std::string to_string() const  {
        std::stringstream out;
        out << std::hex
            <<  (int)data()
            << std::dec;
        return out.str();
      }

    private:
      int i_ = 0;
      const Alloc* alloc_  = nullptr;
      Size_t my_size_ = 0;
      Addr_t my_addr_ = 0;
    };

    Node_view root() {
      return Node_view(0, this);
    }

    const Node_view root() const {
      return Node_view(0, this);
    }

    std::string summary() const {
      std::stringstream out;
      std::string dashes(80, '-');
      out << dashes << "\n";
      out << "Bytes used: " << util::Byte_r(bytes_used())
          << ", Bytes free: " << util::Byte_r(bytes_free())
          << " H: " << std::dec << tree_height()
          << " W: " << tree_width()
          << " Tree size: " << util::Byte_r(nodes_.size()) << "\n"
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
    std::string draw_tree(bool index = false) const {
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
    const uintptr_t addr_limit_ = 0;
    const Size_t pool_size_ = min_size;
    Size_t bytes_used_ = 0;
    bool overbooked_ = false;
  };

}

#endif
