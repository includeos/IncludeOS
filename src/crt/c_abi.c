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
#include <stdint.h>
#include <stdio.h>
#include <kprint>
#define weak_alias(name, aliasname) _weak_alias (name, aliasname)
#define _weak_alias(name, aliasname) \
    extern __typeof (name) aliasname __attribute__ ((weak, alias (#name)));
void* __dso_handle;

__attribute__((no_sanitize("all")))
uint32_t _move_symbols(void* sym_loc)
{
  // re-align new symbol area to a page boundary
  const intptr_t align_size = 4096 - ((uintptr_t) sym_loc & 4095);
  sym_loc = (void*) ((uintptr_t) sym_loc + align_size);

  extern char _ELF_SYM_START_;
  // read out size of symbols **before** moving them
  extern int  _get_elf_section_datasize(const void*);
  int elfsym_size = _get_elf_section_datasize(&_ELF_SYM_START_);

  // move ELF symbols to safe area
  extern void _move_elf_syms_location(const void*, void*);
  _move_elf_syms_location(&_ELF_SYM_START_, sym_loc);

  // increment elfsym_size to next page boundary
  if (elfsym_size & 4095) elfsym_size += 4096 - (elfsym_size & 4095);
  return align_size + elfsym_size;
}

#define __assert(expr) \
  if (__builtin_expect(expr,0)) { \
    fprintf(stderr,"Expression %s failed %s:%d in %s",#expr , __FILE__, __LINE__, __FUNCTION__); \
    abort(); \
  }

void* __memcpy_chk(void* dest, const void* src, size_t len, size_t destlen)
{
  __assert (len <= destlen);
  return memcpy(dest, src, len);
}

void* __memmove_chk(void* dest, const void* src, size_t len, size_t destlen)
{
  __assert (len <= destlen);
  return memmove(dest, src, len);
}

void* __memset_chk(void* dest, int c, size_t len, size_t destlen)
{
  __assert (len <= destlen);
  return memset(dest, c, len);
}
char* __strcat_chk(char* dest, const char* src, size_t destlen)
{
  size_t len = strlen(dest) + strlen(src) + 1;
  __assert (len <= destlen);
  return strcat(dest, src);
}

__attribute__((format(printf, 2, 3)))
int __printf_chk (int flag, const char *format, ...)
{
  (void) flag;
  va_list ap;
  va_start (ap, format);
  int done = vfprintf (stdout, format, ap);
  va_end (ap);
  return done;
}
int __fprintf_chk(FILE* fp, int flag, const char* format, ...)
{
  (void) flag;
  va_list arg;
  va_start (arg, format);
  int done = vfprintf(fp, format, arg);
  va_end (arg);
  return done;
}
int __vfprintf_chk(FILE* fp, int flag, const char *format, va_list ap)
{
  (void) flag;
  int done;
  done = vfprintf (fp, format, ap);
  return done;
}
int __vsprintf_chk(char* s, int flag, size_t slen, const char* format, va_list args)
{
  (void) flag;
  int res = vsnprintf(s, slen, format, args);
  __assert ((size_t) res < slen);
  return res;
}
int __vsnprintf_chk (char *s, size_t maxlen, int flags, size_t slen,
		                  const char *format, va_list args)
{
  assert (slen < maxlen);
  (void) flags;
  return vsnprintf(s, slen, format, args);
}
__attribute__((format(printf, 4, 5)))
int __sprintf_chk(char* s, int flags, size_t slen, const char *format, ...)
{
  va_list arg;
  int done;
  va_start (arg, format);
  done = __vsprintf_chk(s, flags, slen, format, arg);
  va_end (arg);
  return done;
}
int __snprintf_chk (char *s, size_t maxlen, int flags, size_t slen,
                		 const char *format, ...)
{
  va_list arg;
  int done;

  va_start (arg, format);
  done = __vsnprintf_chk (s, maxlen, flags, slen, format, arg);
  va_end (arg);

  return done;
}

int __isoc99_scanf (const char *format, ...)
{
  va_list arg;
  va_start (arg, format);
  int done = vfscanf(stdin, format, arg);
  va_end (arg);
  return done;
}
__attribute__((format(scanf, 2, 3)))
int __isoc99_sscanf (const char *s, const char *format, ...)
{
  va_list arg;
  int done;
  va_start (arg, format);
  done = vsscanf(s, format, arg);
  va_end (arg);
  return done;
}

// TODO: too complicated to implement
#include <setjmp.h>

__attribute__ ((noreturn, weak))
void __longjmp_chk(jmp_buf env, int val)
{
  longjmp(env, val);
  __builtin_unreachable();
}
