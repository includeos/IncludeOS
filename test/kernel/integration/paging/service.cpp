// -*- C++ -*-
// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
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

#include <os>
//#include <kernel/memory.hpp>
#include <service>
#include <cassert>
#include <iostream>
#include <kernel/memory.hpp>
#include <arch/x86/paging.hpp>
#include <arch/x86/paging_utils.hpp>
#include <random>
#include <vector>

extern "C" void kernel_sanity_checks();
extern void __page_fault(uintptr_t*, uint32_t);
extern void __cpu_dump_regs(uintptr_t*);


using namespace os;
using namespace util;

constexpr int Page_fault = 14;
constexpr int CR2 = 20;

enum class Pfault : uint32_t {
  none = 0,
  present = 1,
  write_failed = 2,
  user_page = 4,
  reserved_bit = 5,
  xd = 16,
  prot_key = 32,
  sgx = 0x8000
};

struct Magic {
  char id = '!';
  int reboots = 0;
  int last_error = 0;
  void* last_access = nullptr;
  Pfault last_code{};
  int i = 0;
  void(*heap_code)() = nullptr;
};

auto magic_loc = 42_TiB;
Magic* magic = (Magic*)magic_loc;
uintptr_t magic_phys_loc = 2_GiB;
char* protected_page { (char*)5_GiB };
uintptr_t protected_page_phys = 142_MiB;

namespace util {
inline namespace bitops {
template<>
struct enable_bitmask_ops<Pfault> {
  using type = std::underlying_type<Pfault>::type;
  static constexpr bool enable = true;
};
template<>
struct enable_bitmask_ops<uint32_t> {
  using type = uint32_t;
  static constexpr bool enable = true;
};

}
}


uint64_t rand64(){
  static std::mt19937_64 mt_rand(time(0));
  return mt_rand();
}

using namespace util;

extern "C" void __cpu_exception(uintptr_t* regs, int error, uint32_t code){
  __page_fault(regs, code);
  __cpu_dump_regs(regs);
  if (error != Page_fault){
    printf("FAIL\n");
    exit(67);
  }

  magic->last_access = (void*)regs[CR2];
  magic->last_error = error;
  magic->last_code = Pfault(code);

  os::reboot();
}

template <typename E>
struct enable_log {
  static constexpr int level = 0;
};

// Logging ON
template<typename E>
typename std::enable_if<enable_log<E>::level != 0>::type
log(const char* msg)
{
  printf("LOG: %s\n", msg);
}

// Logging OFF
template<typename E>
typename std::enable_if<enable_log<E>::level == 0>::type
log(const char* )
{
  printf("NOTHING\n");
}

// Class TCP - which logging knows nothing about
template <int idx>
class TCP {

};

// Template specialization of enable_log for TCP
template <>
struct enable_log <TCP<1>> {
  static constexpr int level = 1;
};


template <typename Sub = char*>

struct C {

  void func2(){
    printf("Universal function \n");
  }

  void func(){
    printf("I'm ignoring my sub \n");
  }

  Sub s;
};

template <>
void C<std::string>::func() {
  s = "hello";
  printf("I am enabled and have a string %s \n", s.c_str());
}

template <>
void C<int>::func() {
  s = 10;
  printf("I am enabled and have an int %i \n", s);

}

void test_template_log() {
  C<std::string> obj;
  C<int> obj2;

  obj.func();
  obj2.func();

  obj.func2();
  obj2.func2();

  using TCP = TCP<1>;
  log<TCP>("Hello TCP log \n");
}


extern x86::paging::Pml4* __pml4;

using Pflag = x86::paging::Flags;

void verify_test_entries(){

  std::vector<uintptr_t> test_entries
  { 0_b, 4_KiB, 8_KiB, 1_MiB, 2_MiB, 100_MiB, 1_GiB, 1002_MiB, 500_GiB, 513_GiB, magic_loc};

  Expects(mem::active_page_size(0LU) == 4_KiB);
  Expects(mem::active_page_size(4_KiB) == 4_KiB);
  Expects(mem::active_page_size(200_MiB) == 2_MiB);
  Expects(mem::active_page_size(1_GiB) == 1_GiB);
  Expects(mem::active_page_size(100_GiB) == 1_GiB);
  Expects(mem::active_page_size(550_GiB) == 512_GiB);
}

void verify_integrity(){

  std::cout << "Verifying data integrity across page tables \n";

  // Allocate across both 1_G and 2_MiB border
  uintptr_t near = 884_MiB + 4_KiB;
  uintptr_t far_distance = 2_GiB;

  mem::Map far;
  far.lin   = near + far_distance;
  far.phys  = near;
  far.flags = mem::Access::read | mem::Access::write;
  far.size  = 100_MiB;
  far.page_sizes = mem::Map::any_size;

  // Make room by resizing heap
  // TODO: This shouldn't be necessary
  auto heap_key = os::mem::vmmap().in_range(near);
  os::mem::vmmap().resize(heap_key, 100_MiB);

  auto res = mem::map(far);
  Expects(res and res.size == far.size);
  Expects(res.flags == far.flags);
  Expects(res.page_sizes == (2_MiB | 4_KiB));

  std::cout << "* Populating near memory with " << util::Byte_r(res.size) << " random data\n";
  uintptr_t* near_ptr = (uintptr_t*)near;
  //memset(near_ptr, rand64(), range_size);*/
  auto val = rand64();

  size_t count = res.size / sizeof(val);
  std::fill(near_ptr, near_ptr + count, val);

  uintptr_t bytes_ok = 0;
  for (uintptr_t i = 0; i < count; i++){
    Expects(near_ptr[i] == val);
    bytes_ok += sizeof(uintptr_t);
  }

  bytes_ok = 0;
  uintptr_t* far_ptr = (uintptr_t*)far.lin;
  for (uintptr_t i = 0; i < count; i++){
    Expects(near_ptr[i] == val);
    Expects(far_ptr[i] == near_ptr[i]);
    bytes_ok += sizeof(uintptr_t);
  }

  std::cout << "* "<< util::Byte_r(bytes_ok) << " bytes verified OK\n";
  Expects(bytes_ok == far.size);
  std::cout << "* Consistency check OK\n";
}




void verify_magic() {

  printf("Verifying magic\n");
  magic = (Magic*)42_TiB;
  Magic* magic_phys = (Magic*)magic_phys_loc;
  auto m = __pml4->map_r({magic_loc, (uintptr_t)magic_phys,
        Pflag::writable | Pflag::present | Pflag::huge, 4_KiB});
  Expects(m);
  Expects(m.page_sizes == mem::active_page_size(magic));
  Expects(m.page_sizes == 4_KiB);
  Expects(m.size == 4_KiB);
  Expects(m.lin  == magic_loc);
  Expects(m.phys == (uintptr_t)magic_phys);

  if (magic_phys->id != '!') {
    *magic = Magic();
    magic->i = rand() % 4_KiB;
  } else {
    magic->reboots++;
  }
  printf("* Magic OK\n");
}


std::random_device randz;

uint64_t randomz64(){
  static std::mt19937_64 mt_rand(time(0));
  return mt_rand();
}


uint64_t randomz(){
  static std::mt19937 mt_rand(time(0));
  return mt_rand();
}

std::vector<uint64_t> randomz(int n){
  std::vector<uint64_t> v;
  for (int i = 0; i < n; i++)
    v.push_back(randomz64());

  return v;
}

void memmap_vs_pml4()
{

    printf("\n*** Memory map: ***\n");
    auto& mmap = os::mem::vmmap();
    for (auto& r : mmap){
      std::cout << r.second.to_string() << "\n";
    }

    int match = 0;
    const int ranges = 100;
    auto randz = randomz(ranges);
    auto t1 = os::nanos_since_boot();
    for (auto rz : randz)
    {
      if (mmap.in_range(rz)) {
        //printf("mmap: 0x%lx YES\n", rz);
        match++;
      }else {
        //printf("mmap: 0x%lx NO\n", rz);
      }

    }
    auto t = os::nanos_since_boot() - t1;
    printf("Tested %i ranges in %li us. %i matches. \n", ranges, t, match);

    match = 0;
    t1 = os::nanos_since_boot();
    for (auto rz : randz)
    {
      auto* ent = __pml4->entry_r(rz);
      if (ent != nullptr && __pml4->addr_of(*ent) != 0) {
        //printf("__pml4: 0x%lx YES\n", rz);
        match++;
      }else {
        //printf("__pml4: 0x%lx NO\n", rz);
      }
    }
    t = os::nanos_since_boot() - t1;
    printf("Tested %i ranges in %li ns. %i matches. \n", ranges, t, match);

}


void map_non_aligned(){

  std::cout << "Verifying non-aligned mappings fail gracefully\n";
  std::cout << "* Allowed page sizes: " << mem::page_sizes_str(mem::supported_page_sizes()) << "\n";
  auto psize = bits::keeplast(mem::supported_page_sizes());

  auto far_addr1 = 222_GiB;
  auto far_addr2 = 223_GiB;
  auto near_addr1 = 170_MiB + 4_KiB;
  auto near_addr2 = 170_MiB + 8_KiB;

  auto errors = 0;

  std::cout << "* Mapping a " << util::Byte_r(psize) << " page to "
            << Byte_r(near_addr1) << ", no page size restrictions \n";

  // OK - we don't supply page size, only size
  auto res = mem::map({far_addr1, near_addr1, mem::Access::read | mem::Access::write, psize});
  Expects(res);
  Expects(res.size == psize);
  Expects(res.page_sizes & 4_KiB);
  char* far_ptr = (char*) far_addr1;
  char* near_ptr = (char*) near_addr1;
  far_ptr[42] = '!';
  Expects(far_ptr[42] == '!');
  Expects(near_ptr[42] == '!');

  std::cout << "* Mapping a " << util::Byte_r(psize) << " page to "
            << Byte_r(near_addr2) << ", requiring page size " << Byte_r(psize) << "\n";
  try {
    mem::map({far_addr2, near_addr2, mem::Access::read | mem::Access::write, psize, psize});
  } catch (mem::Memory_exception& e) {
    Expects(std::string(e.what()).find(std::string("linear and physical must be aligned to requested page size")));
    std::cout << "* Exception caught as expected\n";
    errors++;
  }

  Expects(errors == 1);

}


int main()
{
  void(*heap_code)() = (void(*)()) malloc(42);

  Expects(Byte_r{std::numeric_limits<int>::max()}.to_string() == "2.000_GiB");
  Expects(Byte_r{std::numeric_limits<uintptr_t>::max()}.to_string() == "16777216.000_TiB");

  verify_magic();
  verify_integrity();
  map_non_aligned();

  Expects(os::mem::active_page_size(0LU) == 4_KiB);


  os::mem::Map prot;
  prot.lin         = (uintptr_t) protected_page;
  prot.phys        = (uintptr_t) protected_page_phys;
  prot.size        = 4_KiB;
  prot.page_sizes  = 4_KiB;
  prot.flags       = mem::Access::read | mem::Access::write;

  mem::Map mapped;
  int expected_reboots = 4;
  if (magic->reboots < expected_reboots) {
    std::cout << "Protection fault test setup\n";
    std::cout << "* Mapping protected page @ " << prot << "\n";
    mapped = mem::map(prot, "Protected test page");
    mem::protect_range((uint64_t)protected_page, mem::Access::read | mem::Access::write);
    Expects(mapped && mapped == prot);
  }

  auto pml3 = __pml4->page_dir(__pml4->entry(magic_loc));
  auto pml2 = pml3->page_dir(pml3->entry(magic_loc));
  auto pml1 = pml2->page_dir(pml2->entry(magic_loc));

  // Write-protect
  if (magic->reboots == 0) {

    pml3 = __pml4->page_dir(__pml4->entry(mapped.lin));
    pml2 = pml3->page_dir(pml3->entry(mapped.lin));
    pml1 = pml2->page_dir(pml2->entry(mapped.lin));

    protected_page[magic->i] = 'a';
    mem::protect_range((uint64_t)protected_page, mem::Access::read);
    Expects(protected_page[magic->i] == 'a');
    std::cout << "* Writing to write-protected page, expecting page write fail\n\n";
    protected_page[magic->i] = 'b';
  }

  if (magic->reboots == 1) {
    Expects(magic->last_error = Page_fault);
    Expects(magic->last_code == (Pfault::present | Pfault::write_failed));
    Expects(protected_page[magic->i] == 'a');
    protected_page[magic->i] = 'b';
    Expects(protected_page[magic->i] == 'b');
    Expects(magic->last_access == &protected_page[magic->i]);
    printf("\n%i WRITE protection PASSED\n", magic->reboots);

    // Read-protect (e.g. not present)
    std::cout << "* Reading non-present page, expecting page read fail\n\n";
    mem::protect_range((uint64_t)protected_page, mem::Access::none);
    Expects(protected_page[magic->i] == 'b');
  }

  if (magic->reboots == 2) {
    // Verify read-protect
    Expects(magic->last_error = Page_fault);
    Expects(magic->last_code == Pfault::none);
    Expects(magic->last_access == &protected_page[magic->i]);
    Expects(protected_page[magic->i] == 'b');
    printf("\n%i READ protection PASSED\n", magic->reboots);

    // Execute protected page
    std::cout << "* Executing code from execute-protected page, expecting instruction fetch fail\n\n";
    mem::protect_range((uint64_t)protected_page, mem::Access::read);
    ((void(*)())(&protected_page[magic->i]))();
  }

  if (magic->reboots == 3){
    // Verify XD
    magic->heap_code = (void(*)()) malloc(42);
    Expects(magic->last_error = Page_fault);
    Expects(magic->last_code == (Pfault::present | Pfault::xd));
    Expects(magic->last_access == &protected_page[magic->i]);
    printf("\n%i EXECUTE protection 1/2 PASSED\n", magic->reboots);
    printf("* Executing heap code @ %p, expecting instruction fetch fail\n\n", magic->heap_code);
    magic->heap_code();
  }

  if (magic->reboots == 4) {
    Expects(magic->last_error = Page_fault);
    Expects(magic->last_code == (Pfault::present | Pfault::xd));

    // Expect last access to be on the same page as heap_code
    auto aligned_last = (uintptr_t)magic->last_access & ~(4_KiB - 1);
    auto aligned_heap = (uintptr_t)magic->heap_code & ~(4_KiB - 1);
    Expects(aligned_last == aligned_heap);

    printf("\n%i EXECUTE protection 2/2 PASSED\n", magic->reboots);
    exit(0);
  }
  exit(67);
}
