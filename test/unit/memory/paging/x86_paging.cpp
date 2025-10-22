// -*-C++-*-
// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 IncludeOS AS, Oslo, Norway
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
#include <os>
#include <arch/x86/paging.hpp>
#include <arch/x86/paging_utils.hpp>
#include <kernel/memory.hpp>
#include <cmath>

using namespace util;

CASE("x86::paging: PML4 Page_dir entry helpers") {
  using namespace x86::paging;
  EXPECT(Pml4::is_page_aligned(0));
  EXPECT(not Pml4::is_page_aligned(4_KiB));
  EXPECT(not Pml4::is_page_aligned(2_MiB));
  EXPECT(not Pml4::is_page_aligned(1_GiB));
  EXPECT(Pml4::is_page_aligned(512_GiB));
  EXPECT(not Pml4::is_page_aligned(4_KiB + 17));

  EXPECT(Pml4::is_range_aligned(0));
  EXPECT(Pml4::is_range_aligned(512_GiB * 512));
  EXPECT(not Pml4::is_range_aligned(512_GiB * 4));

  // Nothing is a page in PML4
  EXPECT(not Pml4::is_page(0));
  EXPECT(not Pml4::is_page(4_KiB));
  EXPECT(not Pml4::is_page(4_KiB + 42_b));
  EXPECT(not Pml4::is_page(4_KiB   | Flags::present | Flags::huge));
  EXPECT(not Pml4::is_page(2_MiB   | Flags::present | Flags::huge));
  EXPECT(not Pml4::is_page(1_GiB   | Flags::present | Flags::huge));
  EXPECT(not Pml4::is_page(512_GiB | Flags::huge));
  EXPECT(not Pml4::is_page(512_GiB));
  EXPECT(not Pml4::is_page(0));
  for (auto rnd : test::random) {
    EXPECT(not Pml4::is_page(rnd));
  }

  EXPECT(Pml4::addr_of(4_KiB + 17_b) == 4_KiB);
  EXPECT(Pml4::to_addr(4_KiB + 17_b) == (void*)4_KiB);

  // Only entries with Flags::pdir | Flags::present is a page dir
  EXPECT(not Pml4::is_page_dir(0));
  EXPECT(not Pml4::is_page_dir(512_GiB));
  EXPECT(Pml4::is_page_dir(512_GiB | (uintptr_t)(Flags::pdir | Flags::present)));
  for (auto rnd : test::random) {
    if (Pml4::is_page_dir(rnd))
    {
      EXPECT((rnd & (uintptr_t)(Flags::pdir)));
    } else {
      EXPECT(not (rnd & (uintptr_t)(Flags::pdir)));
    }
  }

  auto fl1 = Flags::pdir | Flags::present;
  auto fl2 = Flags::all;
  EXPECT((Pml4::flags_of(test::random[0] | fl1) & fl1) == fl1);
  EXPECT(Pml4::flags_of(test::random[1] | fl2) == (Flags::all & ~Flags::huge));
}

CASE("x86::paging: PML3 Page_dir entry helpers") {
  using namespace x86::paging;
  EXPECT(Pml3::is_page_aligned(0));
  EXPECT(not Pml3::is_page_aligned(4_KiB));
  EXPECT(not Pml3::is_page_aligned(2_MiB));
  EXPECT(Pml3::is_page_aligned(1_GiB));
  EXPECT(Pml3::is_page_aligned(512_GiB));
  EXPECT(not Pml3::is_page_aligned(4_KiB + 17));

  EXPECT(not Pml3::is_page(0));
  EXPECT(not Pml3::is_page(4_KiB));
  EXPECT(not Pml3::is_page(4_KiB + 42_b));
  EXPECT(not Pml3::is_page(4_KiB   | Flags::present | Flags::huge));
  EXPECT(not Pml3::is_page(2_MiB   | Flags::present));
  EXPECT(not Pml3::is_page(2_MiB   | Flags::present | Flags::huge));
  EXPECT(not Pml3::is_page(1_GiB   | Flags::present));
  EXPECT(Pml3::is_page(1_GiB       | Flags::huge));
  EXPECT(not Pml3::is_page(512_GiB | Flags::present));
  EXPECT(Pml3::is_page(512_GiB     | Flags::huge));

  EXPECT(Pml3::addr_of(4_KiB + 17_b) == 4_KiB);
  EXPECT(Pml3::to_addr(4_KiB + 17_b) == (void*)4_KiB);

  for (auto rnd : test::random) {
    if (Pml3::is_page_dir(rnd))
    {
      EXPECT((rnd & (uintptr_t)(Flags::pdir)));
    } else {
      EXPECT(not (rnd & (uintptr_t)(Flags::pdir)));
    }
  }

  auto fl1 = Flags::pdir | Flags::present;
  auto fl2 = Flags::all;
  EXPECT((Pml3::flags_of(test::random[0] | fl1) & fl1) == fl1);
  EXPECT(Pml3::flags_of(test::random[1] | fl2) == Flags::all);

}

CASE("x86::paging: PML2 Page_dir entry helpers") {
  using namespace x86::paging;
  EXPECT(Pml2::is_page_aligned(0));
  EXPECT(not Pml2::is_page_aligned(4_KiB));
  EXPECT(Pml2::is_page_aligned(2_MiB));
  EXPECT(Pml2::is_page_aligned(1_GiB));
  EXPECT(Pml2::is_page_aligned(512_GiB));
  EXPECT(not Pml2::is_page_aligned(4_KiB + 17));

  EXPECT(not Pml2::is_page(0));
  EXPECT(not Pml2::is_page(4_KiB));
  EXPECT(not Pml2::is_page(4_KiB + 420_b));
  EXPECT(not Pml2::is_page(4_KiB   | Flags::present | Flags::huge));
  EXPECT(not Pml2::is_page(2_MiB   | Flags::present));
  EXPECT(Pml2::is_page(2_MiB       | Flags::present | Flags::huge));
  EXPECT(not Pml2::is_page(1_GiB   | Flags::present));
  EXPECT(Pml2::is_page(1_GiB       | Flags::present | Flags::huge));
  EXPECT(not Pml2::is_page(512_GiB | Flags::present));
  EXPECT(Pml2::is_page(512_GiB     | Flags::present | Flags::huge));

  EXPECT(Pml2::addr_of(4_KiB + 17_b) == 4_KiB);
  EXPECT(Pml2::to_addr(4_KiB + 17_b) == (void*)4_KiB);

  for (auto rnd : test::random) {
    if (Pml2::is_page_dir(rnd))
    {
      EXPECT((rnd & (uintptr_t)(Flags::pdir)));
    } else {
      EXPECT(not (rnd & (uintptr_t)(Flags::pdir)));
    }
  }

  auto fl1 = Flags::pdir | Flags::present;
  auto fl2 = Flags::all;
  EXPECT((Pml2::flags_of(test::random[0] | fl1) & fl1) == fl1);
  EXPECT(Pml2::flags_of(test::random[1] | fl2) == Flags::all);

}

CASE("x86::paging: PML1 Page_dir entry helpers") {
  using namespace x86::paging;
  EXPECT(Pml1::is_page_aligned(0));
  EXPECT(Pml1::is_page_aligned(4_KiB));
  EXPECT(Pml1::is_page_aligned(2_MiB));
  EXPECT(Pml1::is_page_aligned(1_GiB));
  EXPECT(Pml1::is_page_aligned(512_GiB));
  EXPECT(not Pml1::is_page_aligned(4_KiB + 17));

  // Every entry is a page in PML1 unless pdir is set
  // (alignment can't be required due to flags)
  EXPECT(Pml1::is_page(0));
  for (auto rnd : test::random) {
    EXPECT(Pml1::is_page(rnd & (uintptr_t)~Flags::pdir));
  }
  EXPECT(Pml1::is_page(4_KiB));
  EXPECT(Pml1::is_page(5_KiB + 17_b));
  EXPECT(Pml1::is_page(4_KiB       | Flags::present | Flags::huge));
  EXPECT(Pml1::is_page(2_MiB       | Flags::present));
  EXPECT(Pml1::is_page(2_MiB       | Flags::present | Flags::huge));
  EXPECT(Pml1::is_page(1_GiB       | Flags::present));
  EXPECT(Pml1::is_page(1_GiB       | Flags::present | Flags::huge));
  EXPECT(Pml1::is_page(512_GiB     | Flags::present));
  EXPECT(Pml1::is_page(512_GiB     | Flags::present | Flags::huge));

  EXPECT(Pml1::addr_of(4_KiB + 17_b) == 4_KiB);
  EXPECT(Pml1::to_addr(4_KiB + 17_b) == (void*)4_KiB);

  EXPECT(not Pml1::is_page_dir(0));

  for (auto rnd : test::random) {
    EXPECT(not Pml1::is_page_dir(rnd));
  }

  auto fl1 = Flags::pdir | Flags::present;
  auto fl2 = Flags::all;
  EXPECT((Pml1::flags_of(test::random[0] | fl1) & fl1) == Flags::present);
  EXPECT(Pml1::flags_of(test::random[1] | fl2) == (Flags::all & ~(Flags::pdir | Flags::huge)));

}


CASE("x86::paging 4-level x86_64 paging") {
  SETUP ("Initializing page tables") {
    using namespace x86::paging;
    using Pflag = x86::paging::Flags;
    Pml4* __pml4 = new Pml4();

    EXPECT(__pml4 != nullptr);
    EXPECT(__pml4->size() == 512);

    uintptr_t* entries = static_cast<uintptr_t*>(__pml4->data());
    for (int i = 0; i < __pml4->size(); i++)
    {
      EXPECT(entries[i] == 0);
      EXPECT(__pml4->at(i) == entries[i]);
    }

    SECTION("Failing calls to  map_r")
    {
      // Call map_r with 0 page returns empty map
      EXPECT_THROWS(__pml4->map_r({0, 1, Pflag::present, 0_KiB}));
      EXPECT(__pml4->at(0) == 0);
      EXPECT(! __pml4->is_page_dir(__pml4->at(0)));
      EXPECT(__pml4->at(0) == entries[0]);

    }

    SECTION("Calling non-static helpers")
    {
      EXPECT(entries[0] == 0);
      EXPECT(__pml4->at(0) == 0);
      EXPECT(__pml4->within_range(0));
      EXPECT(__pml4->indexof(0) == 0);
    }

    SECTION("Recursively map pages within a single 4k page dir")
    {

      // Recursively map 1k page, creating tables as needed
      auto map = __pml4->map_r({0, 4_KiB, Pflag::present, 1_MiB});

      EXPECT(map);
      EXPECT(__pml4->at(0) != 0);

      x86::paging::Map m;
      EXPECT(not m);
      m.lin       = 0;
      m.phys      = 4_KiB;
      // NOTE: Execute is allowed by default on intel
      m.flags      = Pflag::present;
      m.size       = 1_MiB;
      m.page_sizes = 4_KiB;

      EXPECT(map == m);

      auto flag = __pml4->flags_of(*__pml4->entry(0));
      EXPECT(flag == (Pflag::present | Pflag::pdir));

      // Get PML3
      EXPECT(__pml4->is_page_dir(__pml4->at(0)));
      auto* pml3 = __pml4->page_dir(&__pml4->at(0));
      EXPECT(pml3 != nullptr);

      // Get PML2
      EXPECT(pml3->is_page_dir(pml3->at(0)));
      auto* pml2 = pml3->page_dir(&pml3->at(0));
      EXPECT(pml2 != nullptr);

      // GET PML1
      EXPECT(pml2->is_page_dir(pml2->at(0)));
      auto* pml1 = pml2->page_dir(&pml2->at(0));
      EXPECT(pml1 != nullptr);
      EXPECT(! pml1->is_page_dir(pml1->at(0)));

    }

    SECTION("Recursively map range across many 2Mb page dirs")
    {

      EXPECT(entries[0] == 0);
      EXPECT(__pml4->at(0) == 0);

      const auto msize = (1_GiB - 4_KiB);
      const auto lin = 100_GiB;
      const auto phys = 1_TiB;

      // Recursively map 1k page, creating tables as needed
      auto map = __pml4->map_r({lin, phys, Pflag::present, msize});

      EXPECT(map);

      EXPECT(__pml4->at(0) != 0);

      x86::paging::Map m;
      EXPECT(not m);
      m.lin       = lin;
      m.phys      = phys;
      m.flags     = Pflag::present;
      m.size      = msize;
      m.page_sizes = 2_MiB | 4_KiB;
      EXPECT(map == m);
      EXPECT(map.size == msize);
      EXPECT(map.size < 1_GiB);


      auto flag = __pml4->flags_of(*__pml4->entry(0));
      EXPECT(flag == (Pflag::present | Pflag::pdir));

      // Get PML3
      auto index = __pml4->indexof(100_GiB);
      EXPECT(__pml4->is_page_dir(__pml4->at(0)));
      auto* pml3 = __pml4->page_dir(&__pml4->at(0));
      EXPECT(pml3 != nullptr);

      // Get PML2
      index = pml3->indexof(100_GiB);
      EXPECT(pml3->is_page_dir(pml3->at(index)));
      auto* pml2 = pml3->page_dir(&pml3->at(index));
      EXPECT(pml2 != nullptr);

      // GET PML1
      index = pml2->indexof(100_GiB);
      EXPECT(not pml2->is_page_dir(pml2->at(index)));
      EXPECT_THROWS(pml2->page_dir(&pml2->at(index)));

    }
  }
}


extern x86::paging::Pml4* __pml4;
extern uintptr_t __exec_begin;
extern uintptr_t __exec_end;

void init_default_paging(uintptr_t exec_beg = 0xa00000, uintptr_t exec_end = 0xb0000b){
  __exec_begin = exec_beg;
  __exec_end   = exec_end;

  extern void __arch_init_paging();

  // Initialize default paging (all except actually passing it to CPU)
  if (__pml4 != nullptr) {
    delete __pml4;
    os::mem::vmmap().clear();
  }
  __arch_init_paging();
}


CASE ("x86::paging Verify execute protection")
{
    using namespace util;
    using Access = os::mem::Access;

    init_default_paging(0xa00000, 0xc00000);
    // 4KiB 0-page has no access
    EXPECT(__pml4->active_page_size(0LU) == 4_KiB);
    EXPECT(os::mem::active_page_size(0LU) == 4_KiB);
    EXPECT(os::mem::flags(0) == Access::none);

    auto flags = os::mem::flags(__exec_begin);

    // .text segment has execute + read access up to next 4kb page
    EXPECT(os::mem::flags(__exec_begin) == (Access::execute | Access::read));
    EXPECT(os::mem::flags(__exec_end - 1)   == (Access::execute | Access::read));
    EXPECT(os::mem::flags(__exec_end + 4_KiB) == (Access::read | Access::write));

    for (int i = 0; i < 10; i++ ) {
      auto exec_start = (rand() & ~0xfff);
      auto exec_end = exec_start + rand() % 100_MiB;

      init_default_paging(exec_start, exec_end);

      // 4KiB 0-page has no access
      EXPECT(os::mem::active_page_size(0LU) == 4_KiB);
      EXPECT(os::mem::flags(0) == Access::none);

      // .text segment has execute + read access up to next 4kb page
      EXPECT(os::mem::flags(__exec_begin) == (Access::execute | Access::read));

      EXPECT(os::mem::flags(__exec_end - 1)   == (Access::execute | Access::read));
      EXPECT(os::mem::flags(__exec_end + 4_KiB) == (Access::read | Access::write));
    }
}

CASE("x86::paging controlling page sizes"){
  // Not all CPU's support e.g. 1 GiB pages.
  using namespace util;
  using namespace x86;

  init_default_paging();

  paging::Map req {42_MiB, 10_MiB, paging::Flags::present, 42_MiB, 4_KiB };
  auto res = __pml4->map_r(req);
  EXPECT(res);
  EXPECT(res.page_sizes == 4_KiB);

  req = {42_MiB, 10_MiB, paging::Flags::present, 42_MiB, 2_MiB };
  res = __pml4->map_r(req);

  // We can't get 2_MiB pages when 4k page dirs are already allocated here
  // e.g. we never deallocate page tables with map_r
  // TODO: implment another function to deallocate all page_dirs in a range
  EXPECT(! res);

  req = {42_MiB + 42_MiB, 10_MiB, paging::Flags::present, 42_MiB, 2_MiB };
  res = __pml4->map_r(req);
  EXPECT(__pml4->active_page_size(100_MiB) == 2_MiB);
  EXPECT(res.page_sizes == 2_MiB);
  EXPECT(res.size == req.size);
  EXPECT((res.flags & req.flags) != paging::Flags::none);

  // We can refine to smaller page sizes later (e.g. allocate more tables)
  req = {42_MiB + 42_MiB, 10_MiB, paging::Flags::present, 42_MiB, 4_KiB };
  res = __pml4->map_r(req);
  EXPECT(res);
  EXPECT(res.page_sizes == 4_KiB);
  EXPECT(res.size == req.size);
  EXPECT(__pml4->active_page_size(100_MiB) == 4_KiB);
  EXPECT((res.flags & req.flags) != paging::Flags::none);

  // We can't map a 1G page to 10Mb (alignment fail)
  req = {42_GiB, 10_MiB, paging::Flags::present, 4_GiB, 1_GiB };
  EXPECT_THROWS(__pml4->map_r(req));

  req = {42_GiB, 10_GiB, paging::Flags::present, 4_GiB, os::mem::max_psize() };
  res = __pml4->map_r(req);
  EXPECT(res);
  EXPECT(res.page_sizes == os::mem::max_psize());
  EXPECT(res.size == req.size);
  EXPECT((res.flags & req.flags) != paging::Flags::none);

  // We can't use 512Gb page sizes
  req = {42_GiB, 10_GiB, paging::Flags::present, 4_GiB, 512_GiB };
  EXPECT_THROWS(__pml4->map_r(req));

  // Nor other unsupported page sizes
  req = {42_GiB, 10_MiB, paging::Flags::present, 4_GiB, 3_KiB };
  EXPECT_THROWS(__pml4->map_r(req));

  /**
   * NOTE: Page sizes of e.g. 12_KiB would be interpreted as 0x3000
   * which has 0x1000 set. This would be treated as if 4k pages were requested
   **/
}

CASE ("x86::paging Verify default paging setup")
{
  SETUP("Initialize paging")
  {
    using namespace util;

    using Flags = x86::paging::Flags;
    using Access = os::mem::Access;

    init_default_paging();

    SECTION("Verify page access")
    {
      // 4KiB 0-page has no access
      EXPECT(os::mem::active_page_size(0LU) == 4_KiB);
      EXPECT(os::mem::flags(0) == Access::none);

      // .text segment has execute + read access up to next 4kb page
      EXPECT(os::mem::active_page_size(__exec_begin) == 4_KiB);
      EXPECT(os::mem::flags(__exec_begin) == (Access::execute | Access::read));
      EXPECT(os::mem::flags(__exec_end)   == (Access::execute | Access::read));
      EXPECT(os::mem::flags(__exec_end + 4_KiB) == (Access::read | Access::write));

      // Remaining address space is either read + write or not present
      EXPECT(os::mem::flags(100_MiB)  == (Access::read | Access::write));
      EXPECT(os::mem::flags(1_GiB)    == (Access::read | Access::write));
      EXPECT(os::mem::flags(2_GiB)    == (Access::read | Access::write));
      EXPECT(os::mem::flags(4_GiB)    == (Access::read | Access::write));
      EXPECT(os::mem::flags(8_GiB)    == (Access::read | Access::write));
      EXPECT(os::mem::flags(16_GiB)   == (Access::read | Access::write));
      EXPECT(os::mem::flags(32_GiB)   == (Access::read | Access::write));
      EXPECT(os::mem::flags(64_GiB)   == (Access::read | Access::write));
      EXPECT(os::mem::flags(128_GiB)  == (Access::read | Access::write));
      EXPECT(os::mem::flags(256_GiB)  == (Access::read | Access::write));
      EXPECT(os::mem::flags(512_GiB)  == (Access::none));
      EXPECT(os::mem::flags(1_TiB)    == (Access::none));
      EXPECT(os::mem::flags(128_TiB)  == (Access::none));
      EXPECT(os::mem::flags(256_TiB)  == (Access::none));
      EXPECT(os::mem::flags(512_TiB)  == (Access::none));
      EXPECT(os::mem::flags(1024_TiB) == (Access::none));

    }


    SECTION("Try to break stuff")
    {

      // Ensure ranges
      EXPECT(__pml4->within_range(255_TiB));
      EXPECT(not __pml4->within_range(1000_TiB));
      EXPECT(not __pml4->within_range(std::numeric_limits<uintptr_t>::max()));
      auto* pml3 = __pml4->page_dir(__pml4->entry(0));
      EXPECT(pml3 != nullptr);
      EXPECT(pml3->within_range(512_GiB - 1_b));

      // Lookup and dereference corner cases
      EXPECT(__pml4->entry(0)  != nullptr);
      EXPECT((*__pml4->entry(0) & 1));
      EXPECT(__pml4->entry(1)  == __pml4->entry(0));
      EXPECT(__pml4->entry(4_KiB - 1_b)  == __pml4->entry(0));
      EXPECT(__pml4->entry(255_TiB)  != nullptr);
      EXPECT(*__pml4->entry(255_TiB)  == 0);
      EXPECT(__pml4->entry(512_GiB * 512)  == nullptr);
      EXPECT(__pml4->entry(1000_TiB) == nullptr);
      EXPECT(__pml4->entry(std::numeric_limits<uintptr_t>::max()) == nullptr);

      // Lookup and dereference corner cases recursively
      EXPECT(__pml4->entry_r(0)  != nullptr);
      EXPECT((*__pml4->entry_r(0)) == 0);
      EXPECT(__pml4->entry_r(1)  == __pml4->entry_r(0));
      EXPECT(__pml4->entry_r(4_KiB - 1_b)  == __pml4->entry_r(0));
      EXPECT(__pml4->entry_r(5_KiB)  == __pml4->entry_r(4_KiB));
      EXPECT(__pml4->entry_r(255_TiB)  != nullptr);
      EXPECT(__pml4->entry_r(255_TiB)  == __pml4->entry(255_TiB));
      EXPECT(*__pml4->entry_r(255_TiB)  == 0);
      EXPECT(__pml4->entry_r(512_GiB * 512)  == nullptr);
      EXPECT(__pml4->entry_r(1000_TiB) == nullptr);
      EXPECT(__pml4->entry_r(std::numeric_limits<uintptr_t>::max()) == nullptr);

      // Get flags recursively from corner cases
      EXPECT(__pml4->flags_r(0) == Flags::none);
      EXPECT(__pml4->flags_r(1) == Flags::none);
      EXPECT(__pml4->flags_r(2_MiB) != Flags::none);
      EXPECT(__pml4->flags_r(255_TiB) == Flags::none);
      EXPECT(__pml4->flags_r(512_GiB * 512) == Flags::none);
      EXPECT(__pml4->flags_r(1000_TiB) == Flags::none);
      EXPECT(__pml4->flags_r(std::numeric_limits<uintptr_t>::max()) == Flags::none);

      auto increment = 1_MiB + 44_KiB;

      // Map 4k-aligned sizes
      auto addr = (rand() & ~(4_KiB -1));
      auto map = __pml4->map_r({addr, addr, Flags::present, increment});

      auto summary_pre = __pml4->summary();
      auto* pml3_ent2  = __pml4->entry(513_GiB);
      EXPECT(pml3_ent2 != nullptr);
      EXPECT(!__pml4->is_page_dir(*pml3_ent2));

      // Allocate large amounts of page tables
      auto full_map = x86::paging::Map();
      full_map.page_sizes = 4_KiB;
      for (auto i = 513_GiB; i < 514_GiB; i += increment)
        {
          auto map = __pml4->map_r({i, i, Flags::present, increment});
          auto old = full_map;
          EXPECT(map);
          full_map += map;
          EXPECT(full_map.size > old.size);
        }

      EXPECT(__pml4->entry_r(513_GiB) != nullptr);
      EXPECT(! __pml4->is_page_dir(*__pml4->entry_r(513_GiB)));

      EXPECT(full_map.size == bits::roundto(increment, 1_GiB));
      EXPECT(full_map.page_sizes == 4_KiB);

      int page_dirs_found = 0;
      int kb_pages_found = 0;

      // Descend depth first into every page directory pointer
      using Tbl = std::array<uintptr_t, 512>;
      Tbl& tbl4 = *(Tbl*)__pml4->data();

#ifdef DEBUG_UNIT
      std::cout << __pml4->summary(true, 0) << "\n";
#endif

      // PML4
      for (auto& ent4 : tbl4 ) {
        if (ent4 & Flags::pdir) {
          page_dirs_found++;
          auto* sub3 = __pml4->page_dir(&ent4);
          EXPECT(sub3);

          // Expect the first pml3 to have a mapped 0-entry
          if ((void*)&ent4 == (void*)&tbl4) {
            EXPECT(sub3->at(0) != 0);
          }

          EXPECT_THROWS(sub3->at(512));
          Tbl& tbl3 = *(Tbl*)sub3->data();
          // PML3
          for (auto& ent3 : tbl3 ) {
            if (ent3 & Flags::pdir) {
              page_dirs_found++;
              auto* sub2 = sub3->page_dir(&ent3);
              EXPECT(sub2);
              EXPECT(sub2->at(0) != 0);
              EXPECT_THROWS(sub2->at(512));
              Tbl& tbl2 = *(Tbl*)sub2->data();
              // PML2
              for (auto& ent2 : tbl2 ) {
                if (ent2 & Flags::pdir) {
                  page_dirs_found++;
                  auto* sub1 = sub2->page_dir(&ent2);
                  EXPECT(sub1);
                  EXPECT_THROWS(sub1->at(512));
                  Tbl& tbl1 = *(Tbl*)sub1->data();
                  // PML1
                  for (auto& ent1 : tbl1 ) {
                    if (has_flag(x86::paging::Pml1::flags_of(ent1), Flags::present))
                      kb_pages_found++;
                  }
                }
              }
            }
          }
        }
      };

      auto pml3_hi = __pml4->page_dir(__pml4->entry(513_GiB));
      auto summary_post = __pml4->summary();
      auto sum_pml3 = pml3_hi->summary();
      auto diff_4k = summary_post.pages_4k - summary_pre.pages_4k;

      EXPECT(kb_pages_found == summary_post.pages_4k);
      EXPECT(diff_4k == sum_pml3.pages_4k);
      EXPECT(sum_pml3.pages_1g == 0);
      EXPECT(sum_pml3.pages_2m == 0);

      auto rounded = bits::roundto(increment, 1_GiB);
      EXPECT(rounded % 4_KiB == 0);

      auto rounded_pages = rounded / 4_KiB;
      EXPECT(rounded_pages == sum_pml3.pages_4k);

      auto extra_pages = sum_pml3.pages_4k - rounded_pages;
      EXPECT(extra_pages == 0);

    }
  }
}


CASE ("Map various ranges")
{
  using Flags = x86::paging::Flags;
  init_default_paging();

  x86::paging::Map req;
  x86::paging::Map res;

  req = {5_GiB, 4_GiB, Flags::present, 2_MiB};
  res = __pml4->map_r(req);
  EXPECT(res.size == req.size);
  EXPECT(res.lin  == req.lin);
  EXPECT(res.phys  == req.phys);
  EXPECT(has_flag(res.flags, req.flags));

  req = {513_GiB, 4_GiB, Flags::present, 10_MiB};
  res = __pml4->map_r(req);
  EXPECT(res.size == req.size);
  EXPECT(res.lin  == req.lin);
  EXPECT(res.phys  == req.phys);
  EXPECT(has_flag(res.flags, req.flags));

  req = {590_GiB, 8_GiB, Flags::present, 100_MiB + 42_KiB};
  res = __pml4->map_r(req);
  EXPECT(res.size == bits::roundto(4_KiB,req.size));
  EXPECT(res.lin  == req.lin);
  EXPECT(res.phys  == req.phys);
  EXPECT(has_flag(res.flags, req.flags));

  req = {680_GiB, 10_GiB, Flags::present, 100_GiB + 42_KiB};
  res = __pml4->map_r(req);
  EXPECT(res.size == bits::roundto(4_KiB,req.size));
  EXPECT(res.lin  == req.lin);
  EXPECT(res.phys  == req.phys);
  EXPECT(has_flag(res.flags, req.flags));

  req = {790_GiB + 3_MiB + 4_KiB, 10_GiB + 2_MiB + 16_KiB, Flags::present, 100_MiB + 42_KiB};
  res = __pml4->map_r(req);
  EXPECT(res.size == bits::roundto(4_KiB,req.size));
  EXPECT(res.lin  == req.lin);
  EXPECT(res.phys  == req.phys);
  EXPECT(has_flag(res.flags, req.flags));

  req = {120_TiB + 17_MiB + 4_KiB, 10_GiB + 2_MiB + 16_KiB, Flags::present, 11_MiB + 42_KiB};
  res = __pml4->map_r(req);
  EXPECT(res.size == bits::roundto(4_KiB,req.size));
  EXPECT(res.lin  == req.lin);
  EXPECT(res.phys  == req.phys);
  EXPECT(has_flag(res.flags, req.flags));


}
