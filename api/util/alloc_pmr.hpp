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

#ifndef UTIL_ALLOC_PMR
#define UTIL_ALLOC_PMR

#if __has_include(<experimental/memory_resource>)
#include <experimental/memory_resource>
#include <experimental/vector>
namespace std {
  namespace pmr = std::experimental::pmr;
}
#else
#include <memory_resource>
#include <vector> // For pmr::vector
#endif
#include <delegate>
extern void* aligned_alloc(size_t alignment, size_t size);

namespace os::mem::detail {
  class Pmr_pool;
  using Pool_ptr = std::shared_ptr<Pmr_pool>;
}

namespace os::mem {

  class Pmr_resource;

  class Pmr_pool {
  public:
    static constexpr size_t default_max_resources    = 0xffffff;
    static constexpr size_t resource_division_offset = 2;
    using Resource     = Pmr_resource;
    using Resource_ptr = std::unique_ptr<Pmr_resource, delegate<void(Resource*)>>;

    inline Pmr_pool(size_t capacity,
                    size_t cap_suballoc = 0,
                    size_t max_rescount = default_max_resources);
    inline Pmr_pool() = default;
    inline Resource_ptr get_resource();
    inline void return_resource(Resource* res);
    inline std::size_t resource_capacity();
    inline std::size_t resource_count();
    inline std::size_t total_capacity();
    inline void set_resource_capacity(std::size_t);
    inline void set_total_capacity(std::size_t);
    inline std::size_t allocated();
    inline std::size_t allocatable();
    inline std::size_t alloc_count();
    inline std::size_t dealloc_count();
    inline bool full();
    inline bool empty();

  private:
    detail::Pool_ptr impl;
  };


  class Pmr_resource : public std::pmr::memory_resource {
  public:
    using Pool_ptr = detail::Pool_ptr;
    using Event    = delegate<void(Pmr_resource& res)>;
    inline Pmr_resource(Pool_ptr p);
    inline Pool_ptr pool();
    inline void* do_allocate(std::size_t size, std::size_t align) override;
    inline void do_deallocate (void* ptr, size_t, size_t) override;
    inline bool do_is_equal(const std::pmr::memory_resource&) const noexcept override;
    inline std::size_t capacity();
    inline std::size_t allocatable();
    inline std::size_t allocated();
    inline std::size_t alloc_count();
    inline std::size_t dealloc_count();
    inline bool full();
    inline bool empty();

    /** Fires when the resource has been full and is not full anymore **/
    void on_non_full(Event e){ non_full = e; }

    /** Fires on transition from < N bytes to >= N bytes allocatable **/
    void on_avail(std::size_t N, Event e) { avail_thresh = N; avail = e; }

  private:
    Pool_ptr pool_;
    std::size_t used = 0;
    std::size_t allocs = 0;
    std::size_t deallocs = 0;
    std::size_t avail_thresh = 0;
    Event non_full{};
    Event avail{};
  };

  struct Default_pmr : public std::pmr::memory_resource {
    void* do_allocate(std::size_t size, std::size_t align) override {
      auto* res = aligned_alloc(align, size);
      if (res == nullptr)
        throw std::bad_alloc();
      return res;
    }

    void do_deallocate (void* ptr, size_t, size_t) override {
      std::free(ptr);
    }

    bool do_is_equal (const std::pmr::memory_resource& other) const noexcept override {
      if (const auto* underlying = dynamic_cast<const Default_pmr*>(&other))
        return true;
      return false;
    }
  };

}

#include <util/detail/alloc_pmr.hpp>

#endif
