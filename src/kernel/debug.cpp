// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

//#define DEBUG // Enable debugging
//#define DEBUG2
#include <kernel/elf.hpp>
#include <util/fixedvec.hpp>
#include <cassert>
#include <cstdlib>
#include <map>
#include <stdexcept>
extern void print_backtrace();
extern void* heap_begin;
extern void* heap_end;

static char dbg_write_buffer[512];
#include <unistd.h>
#define DPRINTF(X,...)  { \
        int len = snprintf(dbg_write_buffer, sizeof(dbg_write_buffer), \
        X, ##__VA_ARGS__); write(1, dbg_write_buffer, len); }


struct allocation
{
  allocation() : addr(0) {}
  allocation(char* A, size_t S, void* L1, void* L2)
    :  addr(A), len(S), level1(L1), level2(L2) {}

  char*  addr;
  size_t len;
  void*  level1;
  void*  level2;
};

static int enable_debugging = 1;
static fixedvector<allocation, 16000>  allocs;
static fixedvector<allocation*, 16000> free_allocs;

extern "C"
void _enable_heap_debugging(int enabled)
{
  enable_debugging = enabled;
}

static allocation* find_alloc(char* addr)
{
  for (auto& x : allocs)
  if (x.addr) // nullptr's are free slots
  {
    if (addr >= x.addr && addr < x.addr + x.len)
        return &x; // perfect match or misaligned pointer
  }
  return nullptr;
}

void* operator new (std::size_t len) throw(std::bad_alloc)
{
  void* data = malloc(len);
  if (!data) throw std::bad_alloc();

  if (enable_debugging) {
    enable_debugging = false;
    if (!free_allocs.empty()) {
      auto* x = free_allocs.pop();
      new(x) allocation((char*) data, len,
                        __builtin_return_address(0),
                        __builtin_return_address(1));
    } else {
      allocs.emplace((char*) data, len,
                      __builtin_return_address(0),
                      __builtin_return_address(1));
    }
    enable_debugging = true;
  }
  return data;
}
void* operator new[] (std::size_t n) throw(std::bad_alloc)
{
  return ::operator new (n);
}


inline void deleted_ptr(void* ptr)
{
  if (enable_debugging) {
    auto* x = find_alloc((char*) ptr);
    if (x == nullptr) {
      if (ptr < heap_begin || ptr > heap_end) {
          DPRINTF("[ERROR] Free on invalid non-heap address: %p\n", ptr);
      } else {
          DPRINTF("[ERROR] Possible double free on address: %p\n", ptr);
      }
      print_backtrace();
    }
    else if (x->addr == ptr) {
      // perfect match
      x->addr = nullptr;
      x->len  = 0;
      free_allocs.add(x);
    }
    else if (x->addr != ptr) {
      DPRINTF("[ERROR] Free on misaligned address: %p inside %p:%u",
             ptr, x->addr, x->len);
      print_backtrace();
    }
  }
  free(ptr);
}
void operator delete(void* ptr) throw()
{
  deleted_ptr(ptr);
}
void operator delete[] (void* ptr) throw()
{
  deleted_ptr(ptr);
}

void safe_print_symbol(int N, void* addr)
{
  char _symbol_buffer[512];
  char _btrace_buffer[512];
  auto symb = Elf::safe_resolve_symbol(
              addr, _symbol_buffer, sizeof(_symbol_buffer));
  int len = snprintf(_btrace_buffer, sizeof(_btrace_buffer),
           "-> [%d] %8x + 0x%.3x: %s\n", \
           N, symb.addr, symb.offset, symb.name);\
  write(1, _btrace_buffer, len);
}

void print_heap_allocations()
{
  DPRINTF("Listing %u allocations...\n", allocs.size());
  for (auto& x : allocs) {
    if (x.addr) {
      // entry
      DPRINTF("[%p] %u bytes\n", x.addr, x.len);
      // backtrace
      safe_print_symbol(1, x.level1);
      safe_print_symbol(2, x.level2);
    }
  }
}
