#ifndef MEM_ALLOC_ARENA_HPP
#define MEM_ALLOC_ARENA_HPP

#include "kprint"
#include <cstdint>
#include <cstddef>
#include <list>
#include <algorithm>

namespace os::mem {

struct Region { uintptr_t addr; size_t size; };

class Arena {
public:
  static constexpr size_t PAGE_SZ = 4096;

  Arena(uintptr_t base, size_t size)
    : base_{align_up(base, PAGE_SZ)}, size_{align_down(size, PAGE_SZ)} {
    if (size_) free_.push_back({base_, size_});
  }

  void* allocate(size_t len) noexcept {
    return allocate_aligned(PAGE_SZ, len);
  }

  void* allocate_aligned(size_t alignment, size_t len) noexcept {
    alignment = std::max(alignment, PAGE_SZ);
    len = align_up(len, PAGE_SZ);

    for (auto it = free_.begin(); it != free_.end(); ++it) {
      uintptr_t a = align_up(it->addr, alignment);
      if (a + len > it->addr + it->size) continue;

      // split left
      if (a > it->addr) {
        Region left{it->addr, a - it->addr};
        it->addr = a;
        it->size -= left.size;
        free_.insert(std::upper_bound(free_.begin(), free_.end(), left, cmp), left);
      }

      // split right
      const uintptr_t end = it->addr + len;
      const uintptr_t it_end = it->addr + it->size;
      if (end < it_end) {
        Region right{end, it_end - end};
        it->size = len;
        free_.insert(std::upper_bound(free_.begin(), free_.end(), right, cmp), right);
      }

      void* ret = (void*)it->addr;
      free_.erase(it);
      return ret;
    }
    return nullptr;
  }

  void* allocate_at(void* addr, size_t len) noexcept {
    // assumes region is deallocated
    kprintf("[arena] allocating at %p, %zu bytes", addr, len);

    uintptr_t a = (uintptr_t)addr;
    len = align_up(len, PAGE_SZ);
    if (a < base_ || a + len > base_ + size_) return nullptr;

    for (auto it = free_.begin(); it != free_.end(); ++it) {
      const uintptr_t start = it->addr;
      const uintptr_t end   = start + it->size;
      if (a >= start && a + len <= end) {
        Region left{start, a - start};
        Region right{a + len, end - (a + len)};
        free_.erase(it);
        if (left.size)
          free_.insert(std::upper_bound(free_.begin(), free_.end(), left, cmp), left);
        if (right.size)
          free_.insert(std::upper_bound(free_.begin(), free_.end(), right, cmp), right);
        return addr;
      }
    }
    return nullptr;
  }

  void deallocate(void* addr, size_t len) noexcept {
    if (!addr || !len) return;
    Region r{align_down((uintptr_t)addr, PAGE_SZ), align_up(len, PAGE_SZ)};

    auto it = std::upper_bound(free_.begin(), free_.end(), r, cmp);
    if (it != free_.begin()) {
      auto prev = std::prev(it);
      if (prev->addr + prev->size == r.addr) {
        prev->size += r.size;
        maybe_merge_with_next(prev);
        return;
      }
    }
    it = free_.insert(it, r);
    maybe_merge_with_next(it);
  }

  bool is_range_free(uintptr_t a, size_t len) const noexcept {
    len = align_up(len, PAGE_SZ);
    for (auto& r : free_) {
      if (a >= r.addr && a + len <= r.addr + r.size) {
        return true;
      }
      if (a + len <= r.addr) {
        break;  // past it
      }
    }
    return false;
  }

  size_t bytes_free() const noexcept {
    size_t s = 0;
    for (auto& r : free_) s += r.size;
    return s;
  }

  size_t bytes_used() const noexcept {
    return size_ - bytes_free();
  }

  uintptr_t allocation_end() const noexcept {
    if (free_.empty()) return base_ + size_;
    auto last = std::prev(free_.end());
    if (last->addr + last->size == base_ + size_) return last->addr;
    return base_ + size_;
  }

  void reset() {
    free_.clear();
    if (size_) free_.push_back({base_, size_});
  }

  // alignment helpers
  template<typename T>
  static constexpr T align_up(T x, size_t a) noexcept {
    return (x + a - 1) & ~(T)(a - 1);
  }
  template<typename T>
  static constexpr T align_down(T x, size_t a) noexcept {
    return x & ~(T)(a - 1);
  }

private:
  static inline bool cmp(const Region& a, const Region& b) noexcept {
    return a.addr < b.addr;
  }

  void maybe_merge_with_next(std::list<Region>::iterator it) {
    auto nx = std::next(it);
    if (nx != free_.end() && it->addr + it->size == nx->addr) {
      it->size += nx->size;
      free_.erase(nx);
    }
  }

  uintptr_t base_{0};
  size_t size_{0};
  std::list<Region> free_;
};

} // namespace os::mem

#endif // MEM_ALLOC_ARENA_HPP

