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
#include "../../../../api/kernel/memory.hpp"
#include "../../../../api/arch/x86_paging.hpp"
#include "../../../../api/arch/x86_paging_utils.hpp"
#include <random>
#include <vector>

extern "C" void kernel_sanity_checks();
extern void __page_fault(uintptr_t*, uint32_t);
extern void __cpu_dump_regs(uintptr_t*);


using namespace os;
using namespace util::literals;

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
};

auto magic_loc = 42_TiB;
Magic* magic = (Magic*)magic_loc;
char* protected_page { (char*)5_GiB };

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

  OS::reboot();
}

extern void print_entry(uintptr_t ent);


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

  for (auto ent : test_entries) {
    std::cout << "\nGetting entry 0x" << std::hex << ent << "\n";
    auto leaf_ent = *__pml4->entry_r(ent);
    std::cout << "Leaf: ";
    print_entry(leaf_ent);
    auto pml_ent = *__pml4->entry(ent);
    std::cout << "PML4:";
    print_entry(pml_ent);
    std::cout << "Active page size: " << Byte_r(__pml4->active_page_size(ent)) << "\n";

  }

  Expects(mem::active_page_size(0LU) == 4_KiB);
  Expects(mem::active_page_size(4_KiB) == 4_KiB);
  Expects(mem::active_page_size(200_MiB) == 2_MiB);
  Expects(mem::active_page_size(1_GiB) == 1_GiB);
  Expects(mem::active_page_size(100_GiB) == 1_GiB);
  Expects(mem::active_page_size(550_GiB) == 512_GiB);
}


void verify_magic() {

  printf("Verifying magic\n");
  magic = (Magic*)42_TiB;
  Magic* magic_phys = (Magic*)1_GiB;
  auto m = __pml4->map_r({magic_loc, 1_GiB, Pflag::writable | Pflag::present | Pflag::huge, 4_KiB});
  Expects(m);
  Expects(m.page_size == mem::active_page_size(magic));
  Expects(m.page_size == 4_KiB);
  Expects(m.size == 4_KiB);
  Expects(m.lin  == magic_loc);
  Expects(m.phys == 1_GiB);
  Expects(m.page_count() == 1);

  if (magic_phys->id != '!') {
    *magic = Magic();
    magic->i = rand() % 4_KiB;
  } else {
    magic->reboots++;
  }
  printf("Magic OK\n");
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
    auto& mmap = OS::memory_map();
    for (auto& r : mmap){
      std::cout << r.second.to_string() << "\n";
    }

    int match = 0;
    const int ranges = 100;
    auto randz = randomz(ranges);
    auto t1 = OS::micros_since_boot();
    for (auto rz : randz)
    {
      if (mmap.in_range(rz)) {
        //printf("mmap: 0x%lx YES\n", rz);
        match++;
      }else {
        //printf("mmap: 0x%lx NO\n", rz);
      }

    }
    auto t = OS::micros_since_boot() - t1;
    printf("Tested %i ranges in %li us. %i matches. \n", ranges, t, match);

    match = 0;
    t1 = OS::micros_since_boot();
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
    t = OS::micros_since_boot() - t1;
    printf("Tested %i ranges in %li us. %i matches. \n", ranges, t, match);

}

int main()
{

  Expects(Byte_r{std::numeric_limits<int>::max()}.to_string() == "2.000_GiB");
  Expects(Byte_r{std::numeric_limits<uintptr_t>::max()}.to_string() == "16777216.000_TiB");

  verify_magic();

  Expects(os::mem::active_page_size(0LU) == 4_KiB);

  os::mem::Map prot;
  prot.lin        = (uintptr_t) protected_page;
  prot.phys       = (uintptr_t) protected_page;
  prot.size       = 4_KiB;
  prot.page_size  = 4_KiB;
  prot.flags      = mem::Access::read | mem::Access::write;

  std::cout << "Mapping protected page @ " << prot << "\n";
  auto mapped = mem::map(prot, "Protected test page");
  Expects(mapped && mapped == prot);

  mem::protect((uint64_t)protected_page, mem::Access::read | mem::Access::write);


  // Write-protect
  if (magic->reboots == 0) {
    protected_page[magic->i] = 'a';
    mem::protect((uint64_t)protected_page, mem::Access::read);
    Expects(protected_page[magic->i] == 'a');
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
    mem::protect((uint64_t)protected_page, mem::Access::none);
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
    mem::protect((uint64_t)protected_page, mem::Access::read);
    ((void(*)())(&protected_page[magic->i]))();
  }

  void(*heap_code)() = (void(*)()) malloc(42);
  if (magic->reboots == 3){
    // Verify XD
    Expects(magic->last_error = Page_fault);
    Expects(magic->last_code == (Pfault::present | Pfault::xd));
    Expects(magic->last_access == &protected_page[magic->i]);
    printf("\n%i EXECUTE protection 1/2 PASSED\n", magic->reboots);
    printf("Executing heap code @ %p \n", heap_code);
    heap_code();
  }

  if (magic->reboots == 4) {
    Expects(magic->last_error = Page_fault);
    Expects(magic->last_code == (Pfault::present | Pfault::xd));
    Expects(magic->last_access == heap_code);

    printf("\n%i EXECUTE protection 2/2 PASSED\n", magic->reboots);
    exit(0);
  }
  exit(67);
}
