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

#ifndef UTIL_MINIALLOC_HPP
#define UTIL_MINIALLOC_HPP

#include <common>
#include <util/bitops.hpp>

extern "C" void kprintf(const char*, ...);

namespace util {
namespace alloc {

  struct Allocation {
    void*  ptr = nullptr;
    size_t size = 0;

    bool operator==(const Allocation& other) const noexcept {
      return ptr == other.ptr && size == other.size;
    }

    operator bool() const noexcept {
      return ptr != nullptr and size != 0;
    }
  };

  enum class Lstack_opt : uint8_t {
    merge,
    no_merge
  };

  template <uintptr_t Min = 4096>
  struct Node {
    Node*  next = nullptr;
    size_t size = 0;

    Node(Node* n, size_t s)
      : next(n), size(s)
    {
      Expects(s >= Min);
    }

    void* begin() {
      return this;
    }

    void* end() {
      return reinterpret_cast<std::byte*>(this) + size;
    }

  };

  namespace detail {
    template <Lstack_opt Opt = Lstack_opt::merge, size_t Min = 4096>
    struct Lstack;
  }

 /**
  * A simple allocator mainly intended as backend for other allocators.
  * There are two modes: merge (which implies sort) or no_merge
  * - Uses free blocks as storage for the nodes (no memory overhead). This also
  *   implies that the first part of each free block must be present in memory.
  * - Does not depend on node data surviving an allocation. Also does not
  *   guarantee that double / wrongly sized deallocation is always caught.
  * - Lazily chops smaller blocks off of larger blocks as requested.
  * - Constant time first time allocation of any sized block.

  * Stack-mode:
  * - LIFO stack behavior for reallocating uniformly sized blocks (pop)
  * - Constant time deallocation (push).
  * - Linear search, first fit, for reallocating arbitrarily sized blocks.
  *   The first block larger than the requested size will be chopped to match,
  *   which implies that the size of the blocks will converge on Min and the
  *   number of blocks will converge on total size / Min for arbitrarily sized
  *   allocations.
  *   NOTE: This mode is only suitable for equally sized blocks, or
  *         for allocate-once scenarios.
  *
  * Merge-mode:
  * - Same as stack-mode for allocations.
  * - Merges with connecting nodes on deallocation
  *
  * The allocator *can* be used as a general purpose allocator if merge enabled
  * but will be slow for that purpose. E.g. alloc / dealloc is in linear time
  * in the general case.
  *
  * NOTE: The allocator will not search for duplicates on deallocation,
  *       e.g. won't prevent double free, so that has to be done elsewhere.
  *       It also won't join contigous blocks on deallocation.
  **/
  template <Lstack_opt Opt = Lstack_opt::merge, size_t Min = 4096>
  class Lstack {
  public:

    // Avoid fragmentation at the cost of speed. Implies sort by addr.
    static constexpr bool   merge_on_dealloc = Opt == Lstack_opt::merge;
    static constexpr bool   is_sorted = merge_on_dealloc;
    static constexpr size_t min_alloc = Min;
    static constexpr int    align = Min;

    using Node = util::alloc::Node<Min>;
    static_assert(Min >= sizeof(Node), "Requires Min. size >= node size");

    /** Allocate size bytes */
    void* allocate(size_t size) noexcept { return allocate_front(size).ptr; }

    /** Allocate size from as low an address as possible. Default. */
    Allocation allocate_front(size_t size) noexcept { return impl.allocate_front(size); }

    /** Allocate size from as high an address as possible */
    Allocation allocate_back(size_t size) noexcept { return impl.allocate_back(size); }

    /** Allocate the largest contiguous chunk of memory */
    Allocation allocate_largest() noexcept { return impl.allocate_largest(); }


    /** Deallocate **/
    void deallocate (void* ptr, size_t size) noexcept(os::hard_noexcept)
    { deallocate({ptr, size}); }

    void deallocate(Allocation a) noexcept(os::hard_noexcept)
    { impl.deallocate(a); }

    /** Donate size memory starting at ptr. to the allocator. **/
    void donate(void* ptr, size_t size) { donate({ptr, size}); }
    void donate(Allocation a) { impl.donate(a); }

    Lstack() = default;
    Lstack(void* mem_begin, size_t size) : impl(mem_begin, size) {};

    /** Lowest donated address */
    uintptr_t pool_begin() const noexcept { return impl.pool_begin(); }

    /** Highest donated address. [pool_begin, pool_end) may not be contiguous */
    uintptr_t pool_end() const noexcept { return impl.pool_end(); }

    /** Sum of the sizes of all donated memory */
    size_t pool_size() const noexcept { return impl.pool_size(); }

    /** Determine if total donated memory forms a contigous range */
    bool is_contiguous() const noexcept(os::hard_noexcept)
    { return impl.is_contiguous(); }

    /* Return true if there are no bytes available */
    bool empty() const noexcept { return impl.empty(); }

    /* Number of bytes allocated total. */
    size_t bytes_allocated() { return impl.bytes_allocated(); }

    /* Number of bytes free, in total. (Likely not contiguous) */
    size_t bytes_free()  const noexcept { return impl.bytes_free(); }

    /** Get first allocated address */
    uintptr_t allocation_begin() const noexcept { return reinterpret_cast<uintptr_t>(impl.allocation_begin()); }

    /**
     * The highest address allocated (more or less ever) + 1.
     * For a non-merging allocator this is likely pessimistic, but
     * no memory is currently handed out beyond this point.
     **/
    uintptr_t allocation_end() { return impl.allocation_end(); }

    ssize_t node_count() { return impl.node_count(); }

    template< class U, class... Args >
    auto make_unique( Args&&... args ) {
      void* addr = allocate(sizeof(U));
      auto deleter = [this](auto* ptr) { this->deallocate(ptr, sizeof(U)); };
      return std::unique_ptr<U, decltype(deleter)>(new (addr) U(std::forward<Args>(args)...), deleter);
    };

  private:
    detail::Lstack<Opt, Min> impl;
  };


  /** Implementation details. Subject to change at any time */
  namespace detail {
    using namespace util;

    template <Lstack_opt Opt, size_t Min>
    struct Lstack {

      // Avoid fragmentation at the cost of speed. Implies sort by addr.
      static constexpr bool   merge_on_dealloc = Opt == Lstack_opt::merge;
      static constexpr bool   is_sorted = merge_on_dealloc;
      static constexpr size_t min_alloc = Min;
      static constexpr int    align = Min;

      using Node = util::alloc::Node<Min>;

      constexpr bool sorted() {
        return is_sorted;
      }

      void* allocate(size_t size){
        return allocate_front(size).ptr;
      }

      Allocation allocate_front(size_t size){
        if (size == 0 or size > bytes_free())
          return {};

        size = bits::roundto(Min, size);
        Ensures(size >= Min);
        auto* node = pop_off(size);
        Allocation a{node, size};
        if (a.ptr != nullptr) {
          Ensures(a.size);
          Ensures(a.size == node->size);
          Ensures(bits::is_aligned(align, (uintptr_t)a.ptr));
          Ensures(bits::is_aligned(align, a.size));
          bytes_allocated_ += a.size;
        }

        return a;
      }

      Allocation allocate_back(size_t size) {
        if (size == 0 or front_ == nullptr)
          return {};
        size = bits::roundto(Min, size);
        Ensures(size >= Min);

        auto** hi_parent = find_highest_fit(size);
        if (hi_parent == nullptr)
          return {};

        auto* hi_fit = *hi_parent;
        Expects(hi_fit->size >= size);

        // Cleanly chop off back node
        if (hi_fit->size == size){
          *hi_parent = hi_fit->next;
          bytes_allocated_ += size;
          return {hi_fit, hi_fit->size};
        }

        // Chop off the end
        Expects(hi_fit->size >= size + min_alloc);
        Allocation chunk {(void*)((uintptr_t)hi_fit->end() - size), size};

        Expects((char*)chunk.ptr >= (char*)hi_fit + size);
        hi_fit->size -= size;
        bytes_allocated_ += size;

        return chunk;
      }

      void deallocate (void* ptr, size_t size) noexcept(os::hard_noexcept) {
        deallocate({ptr, size});
      }

      void deallocate(Allocation a) noexcept(os::hard_noexcept) {
        if (a.ptr == nullptr) // Like POSIX free
          return;

        Expects(bits::is_aligned(Min, (uintptr_t)a.ptr));
        Expects((uintptr_t)a.ptr >= donations_begin_);
        Expects((uintptr_t)a.ptr <= donations_end_ - min_alloc);

        a.size = bits::roundto<Min>(a.size);
        Expects((uintptr_t)a.size <= bytes_allocated());

        push(a.ptr, a.size);
        bytes_allocated_ -= a.size;
      }

      void donate(void* ptr, size_t size){
        Expects(bits::is_aligned(align, (uintptr_t)ptr));
        Expects(bits::is_aligned(align, size));

        push(ptr, size);
        bytes_total_ += size;
        if ((uintptr_t)ptr < donations_begin_ or donations_begin_ == 0)
          donations_begin_ = (uintptr_t)ptr;

        if ((uintptr_t)ptr + size > donations_end_)
          donations_end_ = (uintptr_t)ptr + size;

        Ensures(pool_end() - pool_begin() >= pool_size());
      }

      Lstack(void* ptr, size_t size)
        : front_{nullptr}, bytes_allocated_{0}, bytes_total_{0},
          donations_begin_{0}, donations_end_{0}
      {
        donate(ptr, size);
      }

      Lstack() = default;

      void donate(Allocation a) {
        return donate(a.ptr, a.size);
      }

      const Node* allocation_begin() const noexcept {
        return front_;
      }

      uintptr_t pool_begin() const noexcept {
        return donations_begin_;
      }

      uintptr_t pool_end() const noexcept {
        return donations_end_;
      }

      size_t pool_size() const noexcept {
        return bytes_total_;
      }

      bool is_contiguous() const noexcept(os::hard_noexcept) {
        Expects(pool_end() - pool_begin() >= pool_size());
        return pool_end() - pool_begin() == pool_size();
      }

      bool empty() const noexcept { return front_ == nullptr || front_->size == 0; }

      size_t bytes_allocated() const noexcept
      { return bytes_allocated_; }

      size_t bytes_free() const noexcept
      { return bytes_total_ - bytes_allocated_; }


      /**
       * The highest address allocated (more or less ever) + 1.
       * No memory has been handed out beyond this point
       **/
      uintptr_t allocation_end() {

        if (front_ == nullptr) {
          return donations_end_;
        }

        /*
          If not sorted, this can't be found in a single forward loop:
          Assume we have A in an unordered list [...,A,..].
          We are looking for the highest "missing" range m.
          If A.end > current m, A.end is potentially the beginning of new m.
          But if the rest of the nodes constitute a contiguous segment,
          A.end wasn't a missing node anyway.
        */
        auto* highest_free = find_prior((void*)(donations_end_));
        Expects((uintptr_t)highest_free->end() <= donations_end_);
        if ((uintptr_t)highest_free->end() == donations_end_)
          return (uintptr_t)highest_free->begin();

        if (highest_free->next == nullptr)
          return donations_end_;

        Ensures(not is_sorted);
        return donations_end_;
      }

      Allocation allocate_largest() {
        auto max = pop(find_largest());
        bytes_allocated_ += max.size;
        return max;
      }

      ssize_t node_count(){
        ssize_t res = 0;
        auto* next = front_;
        do {
          if (next == nullptr)
            return res;
          res++;
        } while ((next = next->next));
        return res;
      }

      Node* new_node(void* addr_begin, Node* next, size_t sz){
        Expects((sz & (align - 1)) == 0);
        Expects(((uintptr_t)addr_begin & (align - 1)) == 0);
        return new ((Node*)addr_begin) Node(next, sz);
      }

      void push_front(void* ptr, size_t size) {
        Expects(ptr != nullptr);
        Expects(size > 0);
        if constexpr (merge_on_dealloc) {
            if (front_ != nullptr) {
              Expects(ptr < front_);
              Expects((uintptr_t)ptr + size <= (uintptr_t)front_);
              if ((std::byte*)ptr + size == (std::byte*)front_) {
                front_ = new_node(ptr, front_->next, size + front_->size);
                return;
              }
            }
          }

        if (front_ != nullptr)
          Expects(front_ != ptr);

        auto* old_front = front_;
        front_ = (Node*)ptr;
        new_node(front_, old_front, size);
      }

      void push(void* ptr, size_t size){
        Expects((size & (align - 1)) == 0);

        if constexpr (! merge_on_dealloc) {
            push_front(ptr, size);
          } else {
          auto* prior = find_prior(ptr);

          if (prior == nullptr) {
            push_front(ptr, size);
            return;
          }
          merge(prior, ptr, size);
        }
      }

      Allocation pop(Node** ptr){
        Expects(ptr);
        if (*ptr == nullptr)
          return {};
        auto* node = *ptr;
        *ptr = (*ptr)->next;
        return {node, node->size};
      }

      Node** find_largest(){
        auto** max = &front_;
        auto* next = front_;

        while(next != nullptr) {
          if (next->next == nullptr)
            break;

          if (next->next->size > (*max)->size)
            max = &(next->next);

          next = next->next;
        }
        return max;
      }

      Node** find_highest_fit(size_t size) {
        Expects(size > 0);
        Expects(front_ != nullptr);
        Node** best = nullptr;
        auto*  node = front_;

        if (front_->size >= size)
          best = &front_;

        while (node != nullptr) {
          if (node->next == nullptr)
            break;

          if (node->next->size >= size and
              (best == nullptr or node->next->begin() > *best))
          {
            best = &(node->next);
          }
          node = node->next;
        }
        return best;
      }

      Node* find_prior(void* ptr) {
        auto* node = front_;
        if constexpr (is_sorted) {
            // If sorted we only iterate until next->next > ptr
            if (node >= ptr)
              return nullptr;

            while (node!= nullptr and node->next < ptr
                   and node->next != nullptr)
            {
              node = node->next;
            }
            return node;
          } else {
          // If unsorted we iterate throught the entire list
          Node* best_match = nullptr;
          while (node != nullptr) {
            if (node->begin() < ptr and node->begin() > best_match) {
              best_match = node;
            }
            node = node->next;
          }
          return best_match;
        }
      }

      void merge(Node* prior, void* ptr, size_t size){
        static_assert(merge_on_dealloc, "Merge not enabled");
        Expects(prior);
        // Prevent double dealloc
        Expects((char*)prior + prior->size <= ptr);
        auto* next = prior->next;

        Expects((uintptr_t)ptr + size <= (uintptr_t)next
                or next == nullptr);

        if ((uintptr_t)ptr == (uintptr_t)prior + prior->size) {
          // New node starts exactly at prior end, so merge
          // [prior] => [prior + size ]
          if (next != nullptr)
            Expects((char*)prior + size <= (char*)prior->next);
          prior->size += size;
        } else {
          // There's a gap between prior end and new node
          // [prior]->[prior.next] =>[prior]->[ptr]->[prior.next]
          auto* fresh = new_node(ptr, next, size);
          prior->next = fresh;
          prior = prior->next;
        }

        if (prior->next == nullptr)
          return;

        Expects((uintptr_t)prior + prior->size <= (uintptr_t)prior->next);

        if ((uintptr_t)prior + prior->size == (uintptr_t)prior->next) {
          // We can merge the end of new node with prior.next
          // [prior]->next =>[prior + next.size]
          prior->size += prior->next->size;
          prior->next = prior->next->next;
        }

      }

      Node* pop_off(size_t size) {
        Node* next = front_;
        Node** prev = &front_;
        do {
          if (UNLIKELY(next == nullptr))
            break;

          // Cleanly take out node as-is
          if (next->size == size) {
            *prev = next->next;
            return next;
          }

          // Clip off size from front of node
          if ( next->size > size) {
            *prev = new_node((char*)next + size, next->next, next->size - size);
            next->size = size;
            return next;
          }

          prev = &(next->next);
        } while ((next = next->next));

        // No suitable node
        return nullptr;
      }

      Node* front_ = nullptr;
      ssize_t bytes_allocated_ = 0;
      ssize_t bytes_total_ = 0;
      uintptr_t donations_begin_ = 0;
      uintptr_t donations_end_ = 0;
    };
} // namespace detail
} // namespace util
} // namespace alloc
#endif
