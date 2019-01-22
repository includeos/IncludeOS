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

#define DEBUG_UNIT

#include <common.cxx>
#include <util/alloc_pmr.hpp>
#include <util/units.hpp>

#if __has_include(<experimental/memory_resource>)
#include <experimental/map>
#endif

#include <map>
#include <unordered_map>

CASE("pmr::default_pmr_resource") {

  std::pmr::vector<int> numbers;
  for (int i = 0; i < 1000; i++) {
    numbers.push_back(i * 42);
  }

  EXPECT(numbers.size() == 1000);
}


CASE("pmr::Pmr_pool usage") {
  using namespace util;
  constexpr auto pool_cap = 40_MiB;

  // Using default resource capacity, which is pool_cap / allocator count
  os::mem::Pmr_pool pool{pool_cap, pool_cap};


  EXPECT(pool.total_capacity() == pool_cap);
  auto res = pool.get_resource();
  EXPECT(res->capacity() == pool_cap);

  auto sub_free = res->allocatable();

  std::pmr::polymorphic_allocator<uintptr_t> alloc{res.get()};
  std::pmr::vector<uintptr_t> numbers{alloc};

  EXPECT(numbers.capacity() < 1000);
  numbers.reserve(1000);
  EXPECT(numbers.capacity() == 1000);

  for (auto i : test::random)
    numbers.push_back(i);

  for (auto i = 0; i < numbers.size(); i++)
    EXPECT(numbers.at(i) == test::random.at(i));

  EXPECT(res->allocatable() <= sub_free - 1000);

  // Apparently the same allocator can just be passed to any other container.
  std::pmr::map<int, std::string> maps{alloc};
  maps[42] = "Allocators are interchangeable";
  EXPECT(maps[42] == "Allocators are interchangeable");

  // Allocator storage is kept by the container itself.
  using my_alloc = std::pmr::polymorphic_allocator<uintptr_t>;
  auto unique_alloc = std::make_unique<my_alloc>(res.get());

  std::pmr::vector<std::string> my_strings(*unique_alloc);
  my_strings.push_back("Hello PMR");
  my_strings.push_back("Hello again PMR");

  unique_alloc.reset();
  my_strings.push_back("Still works");
  EXPECT(my_strings.back() == "Still works");

  // Using small res capacity
  constexpr auto alloc_cap = 4_KiB;

  os::mem::Pmr_pool pool2{pool_cap, alloc_cap};
  EXPECT(pool2.total_capacity() == pool_cap);
  EXPECT(pool2.allocatable() == pool_cap);

  auto res2 = pool2.get_resource();
  EXPECT(res2->capacity() == alloc_cap);

  auto sub_free2 = res2->allocatable();

  std::pmr::polymorphic_allocator<int> alloc2{res2.get()};
  std::pmr::vector<int> numbers2{alloc2};

  EXPECT(numbers2.capacity() < 1000);
  numbers2.reserve(1000);
  EXPECT(numbers2.capacity() == 1000);


  EXPECT(res2->allocatable() <= sub_free2 - 1000);
  EXPECT_THROWS(numbers2.reserve(8_KiB));
  numbers2.clear();
  numbers2.shrink_to_fit();

  EXPECT(res2->allocatable() == alloc_cap);
  EXPECT(res2->allocated() == 0);
  EXPECT(pool2.allocatable() == pool_cap);
  EXPECT(pool2.allocated() == 0);

  numbers2.push_back(1);
  EXPECT(numbers2.capacity() > 0);
  EXPECT(numbers2.capacity() < 1000);
  EXPECT(res2->allocatable()  < alloc_cap);
  EXPECT(res2->allocatable()  > alloc_cap - 1000);

}


CASE("pmr::resource usage") {
  using namespace util;

  constexpr auto pool_cap       = 400_KiB;
  constexpr auto resource_cap   = 4_KiB;
  constexpr auto resource_count = 100;

  os::mem::Pmr_pool pool{pool_cap, resource_cap, resource_count};

  // Get resources enought to saturate pool.
  std::vector<os::mem::Pmr_pool::Resource_ptr> resources;

  // Get first resource for comparing it's pool ptr.
  auto res1 = pool.get_resource();
  EXPECT(res1 != nullptr);
  auto pool_ptr = res1->pool();

  resources.emplace_back(std::move(res1));

  // Resources are created on-demand, up to max
  for (int i = 0; i < resource_count - 1; i++) {
    EXPECT(pool.resource_count() == i + 1);
    auto res = pool.get_resource();
    EXPECT(res != nullptr);
    EXPECT(res->capacity() == resource_cap);
    EXPECT(res->allocated() == 0);
    EXPECT(res->pool() == pool_ptr);
    resources.emplace_back(std::move(res));
  }

  // You now can't get more resources
  auto unavail = pool.get_resource();
  EXPECT(unavail == nullptr);

  EXPECT(resources.size() == resource_count);
  EXPECT(pool.resource_count() == resource_count);
  EXPECT(pool_ptr->free_resources() == 0);
  EXPECT(pool_ptr->used_resources() == resource_count);

  std::unordered_map<os::mem::Pmr_resource*, std::vector<void*>> allocations{};

  // Drain all the resources
  for (auto& res : resources) {
    auto exp_alloc = resource_cap;
    EXPECT(not res->full());
    EXPECT(pool.allocatable() >= exp_alloc);
    EXPECT(res->allocatable() == exp_alloc);
    EXPECT(res->allocated() == 0);

    auto* p1 = res->allocate(1_KiB);
    exp_alloc -= 1_KiB;
    EXPECT(res->allocated() == 1_KiB);
    EXPECT(res->capacity() == resource_cap);
    EXPECT(pool.allocatable() >= exp_alloc);
    EXPECT(res->allocatable() == exp_alloc);

    auto* p2 = res->allocate(1_KiB);
    exp_alloc -= 1_KiB;
    EXPECT(res->allocated() == 2_KiB);
    EXPECT(pool.allocatable() >= exp_alloc);
    EXPECT(res->allocatable() == exp_alloc);

    auto* p3 = res->allocate(1_KiB);
    exp_alloc -= 1_KiB;
    EXPECT(res->allocated() == 3_KiB);
    EXPECT(pool.allocatable() >= exp_alloc);
    EXPECT(res->allocatable() == exp_alloc);

    auto* p4 = res->allocate(1_KiB);
    exp_alloc -= 1_KiB;
    EXPECT(res->allocated() == 4_KiB);
    EXPECT(pool.allocatable() >= exp_alloc);
    EXPECT(res->allocatable() == exp_alloc);

    EXPECT(p1 != nullptr);
    EXPECT(p2 != nullptr);
    EXPECT(p3 != nullptr);
    EXPECT(p4 != nullptr);

    allocations[res.get()].push_back(p1);
    allocations[res.get()].push_back(p2);
    allocations[res.get()].push_back(p3);
    allocations[res.get()].push_back(p4);

    EXPECT(res->full());
    EXPECT_THROWS(res->allocate(1_KiB));
    EXPECT(res->full());
  }

  // The pool is now also full
  EXPECT(pool.full());

  // So resources can't allocate more from it.
  // (Pools pmr api is hidden behind implementation for now.)
  EXPECT_THROWS(pool_ptr->allocate(1_KiB));

  int i = 0;
  // Return all resources
  for (auto& res : resources) {
    EXPECT(pool_ptr->free_resources() == i++);
    res.reset();
  }

  for (auto& res : resources) {
    EXPECT(res == nullptr);
  }

  // The resource count remains constant, but all free no used.
  EXPECT(pool.resource_count() == resource_count);
  EXPECT(pool_ptr->free_resources() == resource_count);
  EXPECT(pool_ptr->used_resources() == 0);

  // Pool is still full - deallocations haven't happened
  EXPECT(pool.full());
  EXPECT(pool.allocatable() == 0);

  // NOTE: it's currently possible to deallocate directly to the detail::Pool_ptr
  //       this is an implementation detail prone to change as each allocator's state
  //       won't e updated accordingly.

  for (auto[pool, vec] : allocations)
    for (auto alloc : vec)
      pool->deallocate(alloc, 1_KiB);

  EXPECT(pool.empty());
  EXPECT(not pool.full());
  EXPECT(pool.allocatable() == pool_cap);

  EXPECT(pool.resource_count() == resource_count);
  auto res_reused = pool.get_resource();
  EXPECT(pool.resource_count() == resource_count);

  EXPECT(res_reused->empty());
  EXPECT(res_reused->allocatable() == resource_cap);
  EXPECT(pool_ptr->free_resources() == resource_count - 1);
  EXPECT(pool_ptr->used_resources() == 1);

  res_reused.reset();

  auto res2 = pool.get_resource();

  EXPECT(not res2->full());
  EXPECT(res2->allocatable() == resource_cap);
  auto ptr = res2->allocate(1_KiB);
  EXPECT(ptr != nullptr);
  res2->deallocate(ptr, 1_KiB);
  EXPECT(res2->allocatable() == resource_cap);
}


CASE("pmr::on_non_full event") {
  using namespace util;
  constexpr auto pool_cap = 400_KiB;

  // Using default resource capacity, which is pool_cap / allocator count
  os::mem::Pmr_pool pool{pool_cap, pool_cap};
  auto res = pool.get_resource();
  bool event_fired = false;

  res->on_non_full([&](auto& r){
      EXPECT(&r == res.get());
      EXPECT(not r.full());
      event_fired = true;
    });

  std::pmr::polymorphic_allocator<uintptr_t> alloc{res.get()};
  std::pmr::vector<char> numbers{alloc};
  auto reserved = pool_cap - 2;
  numbers.reserve(reserved);
  EXPECT(numbers.capacity() == reserved);
  EXPECT(res->allocated() == reserved);
  EXPECT(not event_fired);

  numbers.push_back(0);
  numbers.push_back(1);

  // In order to shrink, it needs to allocate new space for 2 chars then copy.
  numbers.shrink_to_fit();
  EXPECT(res->allocated() < reserved);
  EXPECT(event_fired);
  event_fired = false;
  EXPECT(not event_fired);

  for (int i = 2; i < pool_cap / 2; i++) {
    numbers.push_back(i);
  }

  EXPECT(not event_fired);
  EXPECT(not res->full());

  // Reduce capacity, making the resource full right now
  pool.set_resource_capacity(pool_cap / 3);
  numbers.clear();
  numbers.shrink_to_fit();
  EXPECT(event_fired);

}

CASE("pmr::on_avail event") {
  using namespace util;
  constexpr auto pool_cap = 400_KiB;

  // Using default resource capacity, which is pool_cap / allocator count
  os::mem::Pmr_pool pool{pool_cap, pool_cap};
  auto res = pool.get_resource();
  bool event_fired = false;

  res->on_avail(200_KiB, [&](auto& r){
      EXPECT(&r == res.get());
      EXPECT(not r.full());
      EXPECT(r.allocatable() >= 200_KiB);
      event_fired = true;
    });

  std::pmr::polymorphic_allocator<uintptr_t> alloc{res.get()};
  std::pmr::vector<char> numbers{alloc};

  numbers.push_back(0);
  numbers.push_back(1);
  EXPECT(not event_fired);

  auto reserved = 201_KiB;
  numbers.reserve(reserved);
  EXPECT(numbers.capacity() == reserved);
  EXPECT(res->allocated() == reserved);
  EXPECT(not event_fired);

  // In order to shrink, it needs to allocate new space for 2 chars then copy.
  numbers.shrink_to_fit();
  EXPECT(res->allocated() < reserved);
  EXPECT(event_fired);
  event_fired = false;
  EXPECT(not event_fired);

  for (int i = 2; i < 40_KiB; i++) {
    numbers.push_back(i);
  }

  EXPECT(not event_fired);
  EXPECT(not res->full());

  numbers.clear();
  numbers.shrink_to_fit();
  EXPECT(not event_fired);

}


CASE("pmr::default resource cap") {
  // Not providing a resource cap will give each resource a proportion of max

  using namespace util;
  constexpr auto pool_cap = 400_KiB;

  // Using default resource capacity, which is pool_cap / allocator count
  os::mem::Pmr_pool pool{pool_cap};
  auto res1 = pool.get_resource();
  auto expected = pool_cap / (1 + os::mem::Pmr_pool::resource_division_offset);
  EXPECT(res1->allocatable() == expected);

  auto res2 = pool.get_resource();
  expected = pool_cap / (2 + os::mem::Pmr_pool::resource_division_offset);
  EXPECT(res2->allocatable() == expected);

  auto res3 = pool.get_resource();
  expected = pool_cap / (3 + os::mem::Pmr_pool::resource_division_offset);
  EXPECT(res3->allocatable() == expected);

  auto res4 = pool.get_resource();
  expected = pool_cap / (4 + os::mem::Pmr_pool::resource_division_offset);
  EXPECT(res4->allocatable() == expected);


}
