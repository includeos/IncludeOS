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

//#define DEBUG_UNIT

#include <common.cxx>
#include <util/alloc_pmr.hpp>
#include <util/units.hpp>
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
  os::mem::Pmr_pool pool{pool_cap};


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
    auto* p1 = res->allocate(1_KiB);
    auto* p2 = res->allocate(1_KiB);
    auto* p3 = res->allocate(1_KiB);
    auto* p4 = res->allocate(1_KiB);
    EXPECT(p1 != nullptr);
    EXPECT(p2 != nullptr);
    EXPECT(p3 != nullptr);
    EXPECT(p4 != nullptr);

    allocations.at(res.get()).push_back(p1);
    allocations.at(res.get()).push_back(p2);
    allocations.at(res.get()).push_back(p3);
    allocations.at(res.get()).push_back(p4);

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

  EXPECT(not pool.full());
  EXPECT(pool.allocatable() == pool_cap);

  // Each resource's state is remembered as it's passed back and forth.
  // ...There's now no way of fetching any non-full resources
  auto res_tricked = pool.get_resource();

  EXPECT(pool.resource_count() == resource_count);
  EXPECT(res_tricked->full());
  EXPECT(res_tricked->allocatable() == 0);
  EXPECT_THROWS(res_tricked->allocate(1_KiB));

  res_tricked.reset();

  pool_ptr->clear_free_resources();

  auto res2 = pool.get_resource();

  EXPECT(not res2->full());
  EXPECT(res2->allocatable() == resource_cap);
  auto ptr = res2->allocate(1_KiB);
  EXPECT(ptr != nullptr);
  res2->deallocate(ptr, 1_KiB);
  EXPECT(res2->allocatable() == resource_cap);
}


CASE("pmr::Resource performance") {
  using namespace util;
}
