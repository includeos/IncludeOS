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

namespace util {
namespace alloc {

  struct Allocation {
    void* ptr = nullptr;
    size_t size = 0;

    bool operator==(const Allocation& other) {
      return ptr == other.ptr && size == other.size;
    }
  };

  enum class Lstack_opt : uint8_t {
    merge,
    no_merge
  };

  template <uintptr_t Min = 4096>
  struct Node {
    Node* next = nullptr;
    size_t size;

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
    class Lstack;
  }

/**
 * Lazy stack of memory blocks.
 * A stack-like data structure for arbitrarily sized blocks.
 * - Uses free blocks as storage for the nodes (no memory overhead). This also
 *   implies that the first part of each free block must be present in memory.
 * - Lazily chops smaller blocks off of larger blocks as requested.
 * - Constant time first time allocation of any sized block.
 * - LIFO stack behavior for reallocating uniformly sized blocks (pop)
 * - Constant time deallocation (push).
 * - Linear search, first fit, for reallocatiing arbitrarily sized blocks.
 *   The first block larger than the requested size will be chopped to match,
 *   which implies that the size of the blocks will converge on Min and the
 *   number of blocks will converge on total size / Min for arbitrarily sized
 *   allocations.
 *
 * The allocator *can* be used as a general purpose allocator, but will be slow
 * for that purpose. E.g. after n arbitrarily sized deallocations the complexity
 * of allocating a block of size k will be O(n).
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

  using Node = Node<Min>;

  /** Allocate size bytes */
  void* allocate(size_t size) { return impl.allocate(size); }

  /** Allocate the largest contiguous chunk of memory */
  Allocation allocate_largest() { return impl.allocate_largest(); }

  /** Deallocate **/
  void deallocate (void* ptr, size_t size) { impl.deallocate(ptr, size); }
  void deallocate(Allocation a) { impl.deallocate(a); }

  /** Donate size memory starting at ptr. to the allocator. **/
  void donate(void* ptr, size_t size) { impl.donate(ptr, size); }
  void donate(Allocation a) { impl.donate(a); }

  /** Lowest donated address */
  uintptr_t pool_begin() const noexcept { return impl.pool_begin(); }

  /**
   * Highest donated address.
   * @note the range [pool_begin, pool_end] may not be contiguous
   **/
  uintptr_t pool_end() const noexcept { return impl.pool_end(); }

  /* Return true if there are no bytes available */
  bool empty() const noexcept { return impl.empty(); }

  /* Number of bytes allocated total. */
  size_t bytes_allocated() { return impl.bytes_allocated(); }

  /* Number of bytes free, in total. (Likely not contiguous) */
  size_t bytes_free()  const noexcept { return impl.bytes_free(); }

  /** Get first allocated address */
  uintptr_t allocation_begin() const noexcept { return reinterpret_cast<uintptr_t>(impl.begin()); }

  /**
   * The highest address allocated (more or less ever) + 1.
   * For a non-merging allocator this is likely pessimistic, but
   * no memory is currently handed out beyond this point.
   **/
  uintptr_t allocation_end()   { return impl.allocation_end();   }


  ssize_t    node_count()       { return impl.node_count(); }
private:
  detail::Lstack<Opt, Min> impl;
};


namespace detail {
template <Lstack_opt Opt, size_t Min>
class Lstack {
public:

  // Avoid fragmentation at the cost of speed. Implies sort by addr.
  static constexpr bool   merge_on_dealloc = Opt == Lstack_opt::merge;
  static constexpr bool   is_sorted = merge_on_dealloc;
  static constexpr size_t min_alloc = Min;
  static constexpr int    align = Min;

  using Node = Node<Min>;

  void* allocate(size_t size){
    Expects((size & (Min - 1)) == 0);
    auto* node = pop_off(size);
    if (node != nullptr) {
      bytes_allocated_ += node->size;
    }
    return node;
  }

  void deallocate (void* ptr, size_t size){
    push(ptr, size);
    bytes_allocated_ -= size;
  }

  void deallocate(Allocation a) {
    return deallocate(a.ptr, a.size);
  }

  void donate(void* ptr, size_t size){
    push(ptr, size);
    bytes_total_ += size;
    if ((uintptr_t)ptr < donations_begin_ or donations_begin_ == 0)
      donations_begin_ = (uintptr_t)ptr;

    if ((uintptr_t)ptr + size > donations_end_)
      donations_end_ = (uintptr_t)ptr + size;
    printf("Donation: %p -> %#zx, Donations begin: %#zx , end %#zx \n",
           ptr, (std::byte*)ptr + size, donations_begin_, donations_end_);
  }

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

    auto* highest_free = lower_bound((void*)(donations_end_ - min_alloc));
    if ((uintptr_t)highest_free->end() >= donations_end_)
      return (uintptr_t)highest_free->begin();

    if (highest_free->next == nullptr)
      return donations_end_;

    if constexpr (is_sorted) {
      // This must be the second to last node, since this allocator merges
      Expects((uintptr_t)highest_free->next->end() == donations_end_);
      return (uintptr_t)highest_free->next;
    } else {
      return donations_end_;
    }
  }

  Allocation allocate_largest() {
    auto max = pop(find_largest());
    bytes_allocated_ -= max.size;
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
        printf("PUSHD & MERGE\n");
        if (front_ != nullptr and (std::byte*)ptr + size == (std::byte*)front_) {
          front_ = new_node(ptr, front_->next, size + front_->size);
          return;
        }
      }
    printf("PUSH FRONT\n");

    auto* old_front = front_;
    front_ = (Node*)ptr;
    new_node(front_, old_front, size);

  }

  void push(void* ptr, size_t size){
    Expects((size & (align - 1)) == 0);

    if constexpr (! merge_on_dealloc) {
        push_front(ptr, size);
    } else {
      auto* prior = lower_bound(ptr);

      if (prior == nullptr) {
        printf("%p has no prior - push front \n");
        return push_front(ptr, size);
      }
      printf("Prior to %p: %p->%p \n", ptr, prior, (uintptr_t)prior + prior->size);
      merge(prior, ptr, size);
    }
  };

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

  Node* lower_bound(void* ptr) {
    auto* node = front_;
    if (node == nullptr or node > ptr)
      return nullptr;

    //Expects(*node < ptr);
    if constexpr (is_sorted) {
        // If sorted we only iterate until next > ptr
        while (node->next != nullptr and (std::byte*)node->next + node->size < ptr) {
          node = node->next;
        }
        return node;
    } else {
      // If unsorted we iterate throught the entire list
      Node* best_match = node;
      while (node->next != nullptr) {
        if ((std::byte*)node->next + node->size  > ptr) {
          node = node->next;
          continue;
        }
        if (node->next < ptr and node->next > best_match)
          best_match = node;
        node = node->next;
      }
      return best_match;
    }
  }

  void merge(Node* prior, void* ptr, size_t size){
    static_assert(merge_on_dealloc, "Merge not enabled");
    Expects(prior);
    Expects((char*)prior + size <= ptr);
    auto* next = prior->next;
    Expects((uintptr_t)ptr + size >= (uintptr_t)next);

    if ((uintptr_t)ptr == (uintptr_t)prior + prior->size) {
      // New node starts exactly at prior end, so merge
      // [prior] => [prior + size ]
      printf("Merge left \n");
      if (next != nullptr)
        Expects((char*)prior + size <= (char*)prior->next);
      prior->size += size;
    } else {
      // There's a gap between prior end and new node
      // [prior]->[prior.next] =>[prior]->[ptr]->[prior.next]
      printf("Add left \n");
      auto* fresh = new_node(ptr, next, size);
      prior->next = fresh;
      prior = prior->next;
    }

    printf("Merge next? \n");
    if (prior->next == nullptr)
      return;

    Expects((uintptr_t)prior + prior->size <= (uintptr_t)prior->next);

    if ((uintptr_t)prior + prior->size == (uintptr_t)prior->next) {
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
        return nullptr;

      // Cleanly take out node as-is
      if (next->size == size)
      {
        *prev = next->next;
        return next;
      }

      // Clip off size from node
      if ( next->size > size)
      {
        if (next->size - size <= 0 and next->next == nullptr)
          *prev = nullptr;
        else
          *prev = new_node((char*)next + size, next->next, next->size - size);
        next->size = size;
        return next;
      }

      prev = &(next->next);
    } while ((next = next->next));

    // No suitable node
    return nullptr;
  }


  alignas(align) Node* front_ = nullptr;
  ssize_t bytes_allocated_ = 0;
  ssize_t bytes_total_ = 0;
  uintptr_t donations_begin_ = 0;
  uintptr_t donations_end_ = 0;
};
} // namespace detail
} // namespace util
} // namespace alloc
#endif
