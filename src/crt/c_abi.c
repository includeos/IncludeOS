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
#include <sys/reent.h>
#include <malloc.h>
#include <string.h>
#include <kprint>

#define HEAP_ALIGNMENT   63
caddr_t heap_begin;
caddr_t heap_end;

/// IMPLEMENTATION OF Newlib I/O:
#undef stdin
#undef stdout
#undef stderr
// instantiate the Unix standard streams
__FILE* stdin;
__FILE* stdout;
__FILE* stderr;

// stack-protector guard
const uintptr_t __stack_chk_guard = _STACK_GUARD_VALUE_;
extern void panic(const char* why) __attribute__((noreturn));

void _init_c_runtime()
{
  extern char _ELF_SYM_START_;
  extern char _end;

  /// read out size of symbols **before** moving them
  extern int  _get_elf_section_datasize(const void*);
  int elfsym_size = _get_elf_section_datasize(&_ELF_SYM_START_);
  elfsym_size = (elfsym_size < HEAP_ALIGNMENT) ? HEAP_ALIGNMENT : elfsym_size;

  /// move ELF symbols to safe area
  extern void _move_elf_syms_location(const void*, void*);
  _move_elf_syms_location(&_ELF_SYM_START_, &_end);

  /// Initialize .bss section
  extern char _BSS_START_, _BSS_END_;
  streamset8(&_BSS_START_, 0, &_BSS_END_ - &_BSS_START_);

  /// Initialize the heap before exceptions
  // set heap start at end of ELF symbols
  heap_begin = &_end + elfsym_size;
  // cache-align heap, because its not aligned
  heap_begin += HEAP_ALIGNMENT;
  heap_begin = (char*) ((uintptr_t)heap_begin & ~(uintptr_t) HEAP_ALIGNMENT);
  // heap end tracking, used with sbrk
  heap_end   = heap_begin;
  // validate that heap is aligned
  int validate_heap_alignment =
      ((uintptr_t)heap_begin & (uintptr_t) HEAP_ALIGNMENT) == 0;

  /// initialize newlib I/O
  _REENT_INIT_PTR(_REENT);
  // Unix standard streams
  stdin  = _REENT->_stdin;  // stdin  == 1
  stdout = _REENT->_stdout; // stdout == 2
  stderr = _REENT->_stderr; // stderr == 3

  /// initialize exceptions before we can run constructors
  extern char __eh_frame_start[];
  // Tell the stack unwinder where exception frames are located
  extern void __register_frame(void*);
  __register_frame(&__eh_frame_start);

  /// init ELF / backtrace functionality
  extern void _init_elf_parser();
  _init_elf_parser();

  // sanity checks
  assert(heap_begin >= &_end);
  assert(heap_end >= heap_begin);
  assert(validate_heap_alignment);
}

// stack-protector
__attribute__((noreturn))
void __stack_chk_fail(void)
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

#include <setjmp.h>
int _setjmp(jmp_buf env)
{
  return setjmp(env);
}
// linux strchr variant (NOTE: not completely the same!)
void *__rawmemchr (const void *s, int c)
{
  return strchr((const char*) s, c);
}

/// assert() interface of ISO POSIX (2003)
void __assert_fail(const char * assertion, const char * file, unsigned int line, const char * function)
{
  if (function)
    printf("%s:%u  %s: Assertion %s failed.", file, line, function, assertion);
  else
    printf("%s:%u  Assertion %s failed.", file, line, assertion);
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
