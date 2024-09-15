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

#include <stdexcept>
#include <cstdlib>
#include <expects>
#include <kernel/diag.hpp>
#include <kprint>
#include <os>
#include <hal/machine.hpp>
#include "fieldmedic.hpp"

static int diag_bss_arr[1024]{};
static char* heap_42s = nullptr;
static std::span<int> machine_ints;

#define KINFO(FROM, TEXT, ...) kprintf("%13s ] " TEXT "\n", "[ " FROM, ##__VA_ARGS__)
#define MYINFO(X,...) KINFO("Field Medic","⛑️  " X,##__VA_ARGS__)


using namespace medic::diag;

/** Toggles indicating the hook was called by the OS */
static bool diag_post_bss = false;
static bool diag_post_machine_init = false;
static bool diag_post_init_libc = false;

/** Diagnostic hook overrides */
void kernel::diag::post_bss() noexcept {
  diag_post_bss = true;
  Expects(invariant_post_bss());
  MYINFO("BSS Diagnostic passed");
}

bool medic::diag::invariant_post_bss(){
  for (int i : diag_bss_arr) {
    Expects(i == 0);
  }
  return diag_post_bss;
}

void kernel::diag::post_machine_init() noexcept {
  auto mem = (char*)os::machine().memory().allocate(1024);
  Expects(mem != nullptr);
  os::machine().memory().deallocate(mem, 1024);

  os::Machine::Allocator<int> alloc;
  machine_ints = std::span<int>(alloc.allocate(40), 40);
  int j = 0;

  for(auto& i : machine_ints){
    i = j++;
  }

  MYINFO("Machine allocator for %s functional", os::machine().name());
  diag_post_machine_init = true;

  Expects(invariant_post_machine_init());
}


bool medic::diag::invariant_post_machine_init(){
  int j = 0;

  for(auto& i : machine_ints){
    Expects(i == j++);
  }
  return diag_post_machine_init;
}

void kernel::diag::post_init_libc() noexcept {

  default_post_init_libc();
  MYINFO("Elf header intact, global constructors functional");

  heap_42s = (char*)malloc(1024);
  Expects(heap_42s != nullptr);

  std::span<char> chars(heap_42s, 1024);

  for(auto& c : chars){
    c = 42;
  }
  MYINFO("Malloc and brk are functional");
  diag_post_init_libc = true;
  Expects(invariant_post_init_libc());
}

bool medic::diag::invariant_post_init_libc(){
  std::span<char> chars(heap_42s, 1024);

  for(auto& c : chars){
    Expects(c == 42);
  }

  return diag_post_init_libc;
}

namespace medic::diag {

  thread_local std::array<char, diag::bufsize> __tl_bss;
  thread_local std::array<int, 256> __tl_data {
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42,
    42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42, 42};


  static void verify_tls() {
    unsigned correct = 0;
    for (auto& c : __tl_bss) {
      if (c == '!') correct++;
    }

    for (auto& i : __tl_data) {
      if (i == 42) correct++;
    }

    auto expected =  diag::bufsize + __tl_data.size();
    if (correct != expected)
      throw Error("TLS not initialized correctly");
  }

  static int stack_check(int N)
  {
    std::array<char, diag::bufsize> frame_arr;
    memset(frame_arr.data(), '!', diag::bufsize);

    static volatile int random1 = rand();
    if (N) {
      volatile auto res = stack_check(N - 1);
      Expects (res == random1 + N - 1);
    }

    verify_tls();

    for (auto &c : frame_arr) {
      Expects(c == '!');
    }

    return random1 + N;
  }

  template <typename Err>
  static int throw_at(int N) {
    std::array<char, diag::bufsize> frame_arr;
    memset(frame_arr.data(), '!', diag::bufsize);

    if (N) {
      throw_at<Err>(N - 1);
    }

    bool ok = true;
    verify_tls();

    for (auto &c : frame_arr) {
      if (c != '!') ok = false;
      Expects(c == '!');
    }

    if (ok)
      throw Err();

    return -1;
  }

  bool exceptions()
  {
    tls();

    try {
      throw_at<Error>(100);
    }
    catch (const Error& e)
    {
      tls();
      return true;
    }

    return false;
  }

  void init_tls(){
    memset(__tl_bss.data(), '!', diag::bufsize);
  }

  bool stack()
  {
    tls();

    auto check1 = stack_check(10);
    auto check2 = stack_check(100);
    return check1 == check2 - 90;
  }

}
