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
#include <string.h>

#define HEAP_ALIGNMENT   16
caddr_t heap_begin;
caddr_t heap_end;

/// IMPLEMENTATION OF Newlib I/O:
struct _reent newlib_reent;
#undef stdin
#undef stdout
#undef stderr
// instantiate the Unix standard streams
__FILE* stdin;
__FILE* stdout;
__FILE* stderr;

// stack-protector guard
#ifndef _STACK_GUARD_VALUE_
#define _STACK_GUARD_VALUE_ 0xe2dee396
#endif
const uintptr_t __stack_chk_guard = _STACK_GUARD_VALUE_;
extern void panic(const char* why) __attribute__((noreturn));

void _init_c_runtime()
{
  extern char _ELF_SYM_START_;
  extern char _end;
  
  /// init backtrace functionality
  extern void _move_elf_symbols(void*, void*);
  extern void _apply_parser_data(void*);
  extern int  _get_elf_section_size(const void*);
  // first measure the size of symbols
  int symsize = _get_elf_section_size(&_ELF_SYM_START_);
  // estimate somewhere in heap its safe to move them
  char* SYM_LOCATION = &_end + 2 * (4096 + symsize);
  _move_elf_symbols(&_ELF_SYM_START_, SYM_LOCATION);

  // Initialize .bss section
  extern char _BSS_START_, _BSS_END_;
  streamset8(&_BSS_START_, 0, &_BSS_END_ - &_BSS_START_);

  // Initialize the heap before exceptions
  heap_begin = &_end + 0xfff;
  // page-align heap, because its not aligned
  heap_begin = (char*) ((uintptr_t)heap_begin & 0xfffff000);
  // heap end tracking, used with sbrk
  heap_end   = heap_begin;
  // validate that heap is page aligned
  int validate_heap_alignment = ((uintptr_t)heap_begin & 0xfff) == 0;

  /// initialize newlib I/O
  newlib_reent = (struct _reent) _REENT_INIT(newlib_reent);
  // set newlibs internal structure to ours
  _REENT = &newlib_reent;
  // Unix standard streams
  stdin  = _REENT->_stdin;  // stdin  == 1
  stdout = _REENT->_stdout; // stdout == 2
  stderr = _REENT->_stderr; // stderr == 3

  /// initialize exceptions before we can run constructors
  extern void* __eh_frame_start;
  // Tell the stack unwinder where exception frames are located
  extern void __register_frame(void*);
  __register_frame(&__eh_frame_start);

  // move symbols (again) to heap, before calling global constructors
  extern void* _relocate_to_heap(void*);
  void* symheap = _relocate_to_heap(SYM_LOCATION);

  /// call global constructors emitted by compiler
  extern void _init();
  _init();

  // set ELF symbols location here (after initializing everything else)
  _apply_parser_data(symheap);

  // sanity checks
  assert(heap_begin >= &_end);
  assert(heap_end >= heap_begin);
  assert(validate_heap_alignment);
}

// global/static objects should never be destructed here, so ignore this
void* __dso_handle;

// stack-protector
__attribute__((noreturn))
void __stack_chk_fail(void)
{
  panic("Stack protector: Canary modified");
}

// old function result system
int errno = 0;
int* __errno_location(void)
{
  return &errno;
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


int access(const char *pathname, int mode)
{
	(void) pathname;
  (void) mode;

  return 0;
}
char* getcwd(char *buf, size_t size)
{
  (void) buf;
  (void) size;
	return 0;
}
int fcntl(int fd, int cmd, ...)
{
  (void) fd;
  (void) cmd;
	return 0;
}
int fchmod(int fd, mode_t mode)
{
  (void) fd;
  (void) mode;
	return 0;
}
int mkdir(const char *pathname, mode_t mode)
{
  (void) pathname;
  (void) mode;
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
