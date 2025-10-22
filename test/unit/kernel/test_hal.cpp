// -*-C++-*-
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

#include <hal/machine.hpp>
#include <malloc.h>

using namespace util::literals;
struct Pool {
  Pool(size_t size) : size(size) {
    Expects(size);
    data = aligned_alloc(4096, size);
    Expects(data);
  }

  ~Pool(){
    Expects(data);
    free(data);
  }

  void* begin() { return data; }
  void* end() { return (void*)((uintptr_t)data + size); }

  void* data  = nullptr;
  size_t size = 0;
};


CASE("os::Machine instantiation") {

  Pool pool(4_MiB);

  EXPECT(pool.data);
  os::Machine machine(pool.data, pool.size);

  EXPECT(machine.memory().bytes_free() <= pool.size);
  EXPECT(machine.memory().bytes_free() > 3_MiB);

  EXPECT_THROWS(machine.get<Pool>(256));
  EXPECT_THROWS(machine.get<Pool>());

}


CASE("os::Machine basics") {
  Pool pool(4_MiB);

  auto* machine = os::Machine::create(pool.data, pool.size);
  EXPECT(machine != nullptr);
  EXPECT(machine == pool.data);

  auto pool_begin = machine->memory().pool_begin();
  auto pool_end   = machine->memory().pool_end();

  EXPECT((pool_begin >= (uintptr_t)pool.data and pool_begin <= (uintptr_t)pool.data + pool.size));
  EXPECT((pool_end == (uintptr_t)pool.data + pool.size));

  EXPECT(machine->add_new<Pool>(100u) == 0);

  auto& mpool1 = machine->get<Pool>(0);

  EXPECT(mpool1.size == 100);
  auto vec = machine->get<Pool>();
  EXPECT(vec.size() == 1);

  EXPECT(machine->add_new<Pool>(200u) == 1);
  EXPECT(machine->add_new<Pool>(300u) == 2);
  EXPECT(machine->add_new<Pool>(400u) == 3);

  vec = machine->get<Pool>();
  EXPECT(vec.size() == 4);

  int i = 1;
  for (Pool& p : vec) {
    EXPECT(p.size == i++ * 100);
    EXPECT((std::addressof(p) > pool.begin() and std::addressof(p) < pool.end()));
  }

  machine->remove<Pool>(2);
  auto p2 = machine->get<Pool>(2);
  EXPECT(p2.size == 400);

  machine->remove<Pool>(2);
  EXPECT_THROWS(machine->get<Pool>(2));

  machine->remove<Pool>(1);
  machine->remove<Pool>(0);

  EXPECT(machine->get<Pool>().size() == 0);


  // Add std::unique_ptr - ends up in same memory area as add_new
  auto pp1 = std::make_unique<Pool>(1000u);
  auto pp2 = std::make_unique<Pool>(2000u);
  auto pp3 = std::make_unique<Pool>(3000u);

  EXPECT(machine->add<Pool>(std::move(pp1)) == 0);
  EXPECT(machine->add<Pool>(std::move(pp2)) == 1);
  EXPECT(machine->add<Pool>(std::move(pp3)) == 2);

  EXPECT(machine->add_new<Pool>(200u) == 3);
  EXPECT(machine->add_new<Pool>(300u) == 4);
  EXPECT(machine->add_new<Pool>(400u) == 5);

  auto pg0 = machine->get<Pool>(0);
  auto pg1 = machine->get<Pool>(1);
  auto pg2 = machine->get<Pool>(2);
  auto pg3 = machine->get<Pool>(3);
  auto pg4 = machine->get<Pool>(4);
  auto pg5 = machine->get<Pool>(5);

  EXPECT(pg0.size == 1000);
  EXPECT(pg1.size == 2000);
  EXPECT(pg2.size == 3000);
  EXPECT(pg3.size == 200);
  EXPECT(pg4.size == 300);
  EXPECT(pg5.size == 400);

  vec = machine->get<Pool>();
  EXPECT(vec.size() == 6);

  i = 0;
  for (Pool& p : vec) {
    if (i++ < 3)
      EXPECT((std::addressof(p) < pool.begin() or std::addressof(p) > pool.end()));
    else
      EXPECT((std::addressof(p) > pool.begin() and std::addressof(p) < pool.end()));
  }

  for (auto i = 5; i >= 0; i--) {
    machine->remove<Pool>(i);
  }
}
