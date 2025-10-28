#ifndef MEM_ALLOC_HPP
#define MEM_ALLOC_HPP

#include <mem/alloc/buddy.hpp>
#include <mem/allocator.hpp>

namespace os::mem {

  using Raw_allocator = buddy::Alloc<false>;

  /** Get default allocator for untyped allocations */
  Raw_allocator& raw_allocator();

  template <typename T>
  using Typed_allocator = Allocator<T, Raw_allocator>;

  /** Get default std::allocator for typed allocations */
  template <typename T>
  inline Typed_allocator<T> system_allocator() {
    return Typed_allocator<T>(raw_allocator());
  }

  /** True once the heap is initialized and usable */
  bool heap_ready();

} // namespace os::mem

#endif // MEM_ALLOC_HPP
