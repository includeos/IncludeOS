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
#include <util/fixed_vector.hpp>
#include <common>
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <map>
#include <stdexcept>

extern void print_backtrace();
extern void* heap_begin;
extern void* heap_end;
static void safe_print_symbol(int N, void* addr);

static char dbg_write_buffer[1024];
#include <unistd.h>
#define DPRINTF(X,...)  { \
        int L = snprintf(dbg_write_buffer, sizeof(dbg_write_buffer), \
        X, ##__VA_ARGS__); write(1, dbg_write_buffer, L); }


struct allocation
{
  allocation() : addr(0) {}
  allocation(char* A, size_t S, void* L1, void* L2, void* L3)
    :  addr(A), len(S), level1(L1), level2(L2), level3(L3) {}

  char*  addr;
  size_t len;
  void*  level1;
  void*  level2;
  void*  level3;
};

static int enable_debugging = 1;
static int enable_debugging_verbose = 0;
static int enable_buffer_protection = 1;
static Fixed_vector<allocation,  65536> allocs;
static Fixed_vector<allocation*, 65536> free_allocs;

// There is a chance of a buffer overrun where this exact value
// is written, but the chance of that happening is minimal
static const uint64_t buffer_protection_checksum = 0xdeadbeef1badcafe;

extern "C"
void _enable_heap_debugging(int enabled)
{
  enable_debugging = enabled;
}
extern "C"
void _enable_heap_debugging_verbose(int enabled)
{
  enable_debugging_verbose = enabled;
}
extern "C"
void _enable_heap_debugging_buffer_protection(int enabled)
{
  enable_buffer_protection = enabled;
}
extern "C"
int _get_heap_debugging_buffers_usage()
{
  return allocs.size();
}
extern "C"
int _get_heap_debugging_buffers_total()
{
  return allocs.capacity();
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

void* operator new (std::size_t len)
{
  void* data = nullptr;

  if (enable_buffer_protection) {
    // Allocate requested memory + enough to fit checksum at start and end
    data = malloc(len + sizeof(buffer_protection_checksum) * 2);

    // Write checksums
    auto* temp = reinterpret_cast<char*>(data);
    memcpy(temp,
           &buffer_protection_checksum,
           sizeof(buffer_protection_checksum));

    memcpy(temp + sizeof(buffer_protection_checksum) + len,
           &buffer_protection_checksum,
           sizeof(buffer_protection_checksum));

  } else {
    data = malloc(len);
  }

  if (enable_debugging_verbose) {
    DPRINTF("malloc(%llu bytes) == %p\n", (unsigned long long) len, data);
    safe_print_symbol(1, __builtin_return_address(0));
    safe_print_symbol(2, __builtin_return_address(1));
  }

  if (UNLIKELY(!data)) {
      print_backtrace();
      DPRINTF("malloc(%llu bytes): FAILED\n", (unsigned long long) len);
      throw std::bad_alloc();
  }

  if (enable_debugging) {
    if (!free_allocs.empty()) {
      auto* x = free_allocs.pop_back();
      new(x) allocation((char*) data, len,
                        __builtin_return_address(0),
                        __builtin_return_address(1),
                        __builtin_return_address(2));
    } else if (!allocs.free_capacity()) {
      DPRINTF("[WARNING] Internal fixed vectors are FULL, expect bogus double free messages\n");
    } else {
      allocs.emplace_back((char*) data, len,
                      __builtin_return_address(0),
                      __builtin_return_address(1),
                      __builtin_return_address(2));
    }
  }

  if (enable_buffer_protection) {
    // We need to return a pointer to the allocated memory + 4
    // e.g. after our first checksum
    return reinterpret_cast<void*>(reinterpret_cast<char*>(data) +
                                   sizeof(buffer_protection_checksum));
  } else {
    return data;
  }
}
void* operator new[] (std::size_t n)
{
  return ::operator new (n);
}

inline static void deleted_ptr(void* ptr)
{
  if (enable_buffer_protection) {
    // Calculate where the real allocation starts (at our first checksum)
    ptr = reinterpret_cast<void*>(reinterpret_cast<char*>(ptr) -
                                  sizeof(buffer_protection_checksum));
  }

  if (enable_debugging) {
    auto* x = find_alloc((char*) ptr);
    if (x == nullptr) {
      if (ptr < heap_begin || ptr > heap_end) {
          DPRINTF("[ERROR] Free on invalid non-heap address: %p\n", ptr);
      } else {
          DPRINTF("[ERROR] Possible double free on address: %p\n", ptr);
      }
      print_backtrace();
      return;
    }
    else if (x->addr == ptr) {
      if (enable_debugging_verbose) {
        DPRINTF("free(%p) == %llu bytes\n", x->addr, (unsigned long long) x->len);
        safe_print_symbol(1, __builtin_return_address(1));
        safe_print_symbol(2, __builtin_return_address(2));
      }

      // This is the only place where we can verify the buffer protection
      // checksums, since we need to know the length of the allocation
      if (enable_buffer_protection)
      {
        auto* temp = reinterpret_cast<char*>(ptr);
        auto underflow = memcmp(temp,
                                &buffer_protection_checksum,
                                sizeof(buffer_protection_checksum));
        auto overflow = memcmp(temp + sizeof(buffer_protection_checksum) + x->len,
                               &buffer_protection_checksum,
                               sizeof(buffer_protection_checksum));

        if (underflow)
        {
          DPRINTF("[ERROR] Buffer underflow found on address: %p\n", ptr);
          // TODO: print stacktrace
        }

        if (overflow)
        {
          DPRINTF("[ERROR] Buffer overflow found on address: %p\n", ptr);
          // TODO: print stacktrace
        }
      }

      // perfect match
      x->addr = nullptr;
      x->len  = 0;
      free_allocs.push_back(x);
    }
    else if (x->addr != ptr) {
      DPRINTF("[ERROR] Free on misaligned address: %p inside %p:%llu",
             ptr, x->addr, (unsigned long long) x->len);
      print_backtrace();
      return;
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

static void safe_print_symbol(int N, void* addr)
{
  char _symbol_buffer[2048];
  char _btrace_buffer[2048];
  auto symb = Elf::safe_resolve_symbol(
              addr, _symbol_buffer, sizeof(_symbol_buffer));
  int len = snprintf(_btrace_buffer, sizeof(_btrace_buffer),
           "-> [%d] %16p + 0x%.3x: %s\n", \
           N, (void*) symb.addr, symb.offset, symb.name);\
  write(1, _btrace_buffer, len);
}

#include <delegate>
typedef delegate<bool(void*, size_t)> heap_print_func;

void print_heap_allocations(heap_print_func func)
{
  DPRINTF("Listing %u allocations...\n", allocs.size());
  for (auto& x : allocs) {
    if (x.addr != nullptr && func(x.addr, x.len)) {
      // entry
      DPRINTF("[%p] %llu bytes\n", x.addr, (unsigned long long) x.len);
      // backtrace
      safe_print_symbol(1, x.level1);
      safe_print_symbol(2, x.level2);
      safe_print_symbol(3, x.level3);
    }
  }
}
