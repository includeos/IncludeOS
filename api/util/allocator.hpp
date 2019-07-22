// -*- C++ -*-

#ifndef OS_ALLOCATOR_HPP
#define OS_ALLOCATOR_HPP

#include <memory>

namespace os::mem {

  /**
   * C++17 std::allocator interface
   **/
  template <typename T, typename Resource>
  struct Allocator {
    using value_type = T;

    Allocator(Resource& alloc)
      : resource{alloc}
    {}

    template <class U>
    Allocator(const Allocator<U, Resource>& other) noexcept
      : resource{other.resource}
      { }

    T* allocate(std::size_t size) {
      auto res = reinterpret_cast<T*>(resource.allocate(size * sizeof(T)));
      if (res == nullptr)
        throw std::bad_alloc();
      return res;
    }

    void deallocate(T* ptr, std::size_t size) noexcept {
      resource.deallocate(ptr, size * sizeof(T));
    }

    template <typename U>
    struct rebind {
      using other = Allocator<U, Resource>;
    };

    bool operator==(const Allocator& other) const noexcept {
      return resource == other.resource;
    }

    bool operator!=(const Allocator& other) const noexcept {
      return not (other == *this);
    }

    template< class U, class... Args >
    std::unique_ptr<U> make_unique( Args&&... args ) {
      void* addr = allocate(sizeof(U));
      auto deleter = [this](auto* ptr) { deallocate(ptr, sizeof(U)); };
      return std::unique_ptr<U>(new (addr) U(std::forward<Args>(args)...), deleter);
    };

    Resource& resource;
  };
} // namespace

#endif
