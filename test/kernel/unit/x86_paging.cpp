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

#include <common.cxx>
#include <arch/x86_paging.hpp>
#include <arch/x86_paging_utils.hpp>
#include <kernel/memory.hpp>

//#define DEBUG_x86_PAGING
#ifdef DEBUG_x86_PAGING
#define LINK printf("%s:%i: OK\n",__FILE__,__LINE__)
#else
#define LINK (void)
#endif


std::random_device randz;

uint64_t rand64(){
  static std::mt19937_64 mt_rand(time(0));
  return mt_rand();
}

template <int N>
std::vector<uint64_t> rands64()
{
  static std::vector<uint64_t> rnd;

  if (rnd.empty())
  {
    for (int i = 0; i < N; i++)
    {
      rnd.push_back(rand64());
    }
  }

  return rnd;
}

// Random addresses to test for each PML
const std::vector<uint64_t> rands = rands64<10>();

CASE("PML4 Page_dir entry helpers") {
  using namespace x86::paging;
  using namespace util::bitops;
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
  for (auto rnd : rands) {
    EXPECT(not Pml4::is_page(rnd));
  }

  EXPECT(Pml4::addr_of(4_KiB + 17_b) == 4_KiB);
  EXPECT(Pml4::to_addr(4_KiB + 17_b) == (void*)4_KiB);

  // Only entries with Flags::pdir | Flags::present is a page dir
  EXPECT(not Pml4::is_page_dir(0));
  EXPECT(not Pml4::is_page_dir(512_GiB));
  EXPECT(Pml4::is_page_dir(512_GiB | (uintptr_t)(Flags::pdir | Flags::present)));
  for (auto rnd : rands) {
    if (Pml4::is_page_dir(rnd))
    {
      EXPECT((rnd & (uintptr_t)(Flags::pdir)));
    } else {
      EXPECT(not (rnd & (uintptr_t)(Flags::pdir)));
    }
  }

  auto fl1 = Flags::pdir | Flags::present;
  auto fl2 = Flags::all;
  EXPECT((Pml4::flags_of(rands[0] | fl1) & fl1) == fl1);
  EXPECT(Pml4::flags_of(rands[1] | fl2) == (Flags::all & ~Flags::huge));
}

CASE("PML3 Page_dir entry helpers") {
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

  for (auto rnd : rands) {
    if (Pml3::is_page_dir(rnd))
    {
      EXPECT((rnd & (uintptr_t)(Flags::pdir)));
    } else {
      EXPECT(not (rnd & (uintptr_t)(Flags::pdir)));
    }
  }

  auto fl1 = Flags::pdir | Flags::present;
  auto fl2 = Flags::all;
  EXPECT((Pml3::flags_of(rands[0] | fl1) & fl1) == fl1);
  EXPECT(Pml3::flags_of(rands[1] | fl2) == Flags::all);

}

CASE("PML2 Page_dir entry helpers") {
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

  for (auto rnd : rands) {
    if (Pml2::is_page_dir(rnd))
    {
      EXPECT((rnd & (uintptr_t)(Flags::pdir)));
    } else {
      EXPECT(not (rnd & (uintptr_t)(Flags::pdir)));
    }
  }

  auto fl1 = Flags::pdir | Flags::present;
  auto fl2 = Flags::all;
  EXPECT((Pml2::flags_of(rands[0] | fl1) & fl1) == fl1);
  EXPECT(Pml2::flags_of(rands[1] | fl2) == Flags::all);

}

CASE("PML1 Page_dir entry helpers") {
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
  for (auto rnd : rands) {
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

  for (auto rnd : rands) {
    EXPECT(not Pml1::is_page_dir(rnd));
  }

  auto fl1 = Flags::pdir | Flags::present;
  auto fl2 = Flags::all;
  EXPECT((Pml1::flags_of(rands[0] | fl1) & fl1) == Flags::present);
  EXPECT(Pml1::flags_of(rands[1] | fl2) == (Flags::all & ~(Flags::pdir | Flags::huge)));

}


CASE("4-level x86_64 paging") {
  SETUP ("Initializing page tables") {
    using namespace x86::paging;
    using Pflag = x86::paging::Flags;
    Pml4* __pml4 = allocate_pdir<Pml4>();

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
      auto map = __pml4->map_r({0, 1, Pflag::present, 1_MiB});


      EXPECT(map);
      EXPECT(__pml4->at(0) != 0);

      x86::paging::Map m;
      EXPECT(not m);
      EXPECT(m.page_count() == 0);
      m.lin       = 0;
      m.phys      = 1;
      // NOTE: Execute is allowed by default on intel
      m.flags     = Pflag::present;
      m.size      = 1_MiB;
      m.page_size = 4_KiB;

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
      EXPECT(m.page_count() == 0);
      m.lin       = lin;
      m.phys      = phys;
      m.flags     = Pflag::present;
      m.size      = msize;
      m.page_size = 2_MiB | 4_KiB;
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

CASE ("Default paging setup")
{
  SETUP("Initialize paging")
  {
    using namespace util::literals;
    using namespace util::bitops;
    using Flags = x86::paging::Flags;
    using Access = os::mem::Access;

    extern x86::paging::Pml4* __pml4;
    extern uintptr_t __exec_begin;
    extern uintptr_t __exec_end;
    __exec_begin = 0xa00000;
    __exec_end   = 0xb0000b;
    extern void __arch_init_paging();

    // Initialize default paging (all except actually passing it to CPU)
    if (__pml4 == nullptr) {
      __arch_init_paging();
    }


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

      // Map non-aligned sizes
      auto addr = (rand() & ~(4_KiB -1));
      auto map = __pml4->map_r({addr, addr, Flags::present, increment});

      auto summary_pre = __pml4->summary();
      auto* pml3_ent2  = __pml4->entry(513_GiB);
      EXPECT(pml3_ent2 != nullptr);
      EXPECT(!__pml4->is_page_dir(*pml3_ent2));

      // Allocate large amounts of page tables
      auto full_map = x86::paging::Map();
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

      EXPECT(full_map.size == roundto(increment, 1_GiB));
      EXPECT(full_map.page_size == 4_KiB);

      int page_dirs_found = 0;
      int kb_pages_found = 0;

      // Descend depth first into every page directory pointer
      using Tbl = std::array<uintptr_t, 512>;
      Tbl& tbl4 = *(Tbl*)__pml4->data();

      // PML4
      for (auto& ent4 : tbl4 ) {
        if (ent4 & Flags::pdir) {
          page_dirs_found++;
          auto* sub3 = __pml4->page_dir(&ent4);
          EXPECT(sub3);
          EXPECT(sub3->at(0) != 0);
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

      auto rounded = roundto(increment, 1_GiB);
      EXPECT(rounded % 4_KiB == 0);

      auto rounded_pages = rounded / 4_KiB;
      EXPECT(rounded_pages == sum_pml3.pages_4k);

      auto extra_pages = sum_pml3.pages_4k - rounded_pages;
      EXPECT(extra_pages == 0);
    }
  }
}
