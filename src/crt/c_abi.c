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

#include <assert.h>
#include <sys/types.h>
#include <sys/time.h>
#include <util/memstream.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <kprint>

#define HEAP_ALIGNMENT   63
void* heap_begin;
void* heap_end;

void* __dso_handle;

// stack-protector guard
const uintptr_t __stack_chk_guard = (uintptr_t) _STACK_GUARD_VALUE_;
extern void panic(const char* why) __attribute__((noreturn));

extern void (*__preinit_array_start [])();
extern void (*__preinit_array_end [])();
extern void (*__init_array_start [])();
extern void (*__init_array_end [])();

void __libc_init_array() {
  size_t count, i;

  count = __preinit_array_end - __preinit_array_start;
  for (i = 0; i < count; i++)
    __preinit_array_start[i]();

  kprintf("Initializing C constructors \n");
  _init();
  kprintf("Initializing C++ constructors \n");
  count = __init_array_end - __init_array_start;
  for (i = 0; i < count; i++)
    __init_array_start[i]();
}

uint32_t _move_symbols(void* sym_loc)
{
  extern char _ELF_SYM_START_;
  /// read out size of symbols **before** moving them
  extern int  _get_elf_section_datasize(const void*);
  int elfsym_size = _get_elf_section_datasize(&_ELF_SYM_START_);
  elfsym_size = (elfsym_size < HEAP_ALIGNMENT) ? HEAP_ALIGNMENT : elfsym_size;

  /// move ELF symbols to safe area
  extern void _move_elf_syms_location(const void*, void*);
  _move_elf_syms_location(&_ELF_SYM_START_, sym_loc);

  return elfsym_size;
}

static void crt_sanity_checks()
{
  // validate that heap is aligned
  int validate_heap_alignment =
    ((uintptr_t)heap_begin & (uintptr_t) HEAP_ALIGNMENT) == 0;

  extern char _end;
  assert(heap_begin >= (void*) &_end);
  assert(heap_end >= heap_begin);
  assert(validate_heap_alignment);
}


void _init_c_runtime()
{

  /// init ELF / backtrace functionality
  extern void _init_elf_parser();
  _init_elf_parser();
  kprintf("Elf parser initialized \n");
  crt_sanity_checks();
  kprintf("Sanity checks OK \n");
}


// stack-protector
__attribute__((noreturn))
void __stack_chk_fail(void)
{
  panic("Stack protector: Canary modified");
  __builtin_unreachable();
}

__attribute__((noreturn))
void __stack_chk_fail_local(void)
{
  panic("Stack protector: Canary modified");
  __builtin_unreachable();
}

// old function result system
int errno = 0;
int* __errno_location(void)
{
  return &errno;
}


// linux strchr variant (NOTE: not completely the same!)
void *__rawmemchr (const void *s, int c)
{
  return strchr((const char*) s, c);
}


/// sched_yield() causes the calling thread to relinquish the CPU.  The
///               thread is moved to the end of the queue for its static priority
///               and a new thread gets to run.
int sched_yield(void)
{
  // On success, sched_yield() returns 0.  On error, -1 is returned, and
  // errno is set appropriately.
  //errno = ...;
  return -1;
}

void* __memcpy_chk(void* dest, const void* src, size_t len, size_t destlen)
{
  assert (len <= destlen);
  return memcpy(dest, src, len);
}
void * __memset_chk(void* dest, int c, size_t len, size_t destlen)
{
  assert (len <= destlen);
  return memset(dest, c, len);
}

void *aligned_alloc(size_t alignment, size_t size)
{
  return memalign(alignment, size);
}

int access(const char *pathname, int mode)
{
	(void) pathname;
  (void) mode;

  return 0;
}

int fcntl(int fd, int cmd, ...)
{
  (void) fd;
  (void) cmd;
	return 0;
}

int rmdir(const char *pathname)
{
  (void) pathname;
	return 0;
}

int settimeofday(const struct timeval *tv, const struct timezone *tz)
{
  (void) tv;
  (void) tz;
	return 0;
}
