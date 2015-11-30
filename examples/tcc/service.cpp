// Part of the IncludeOS Unikernel  - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud. 
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


#define DEBUG
#define DEBUG2
#include <os>
#include <stdio.h>
#include <string>

#include "libtcc.h"
#include <signal.h>
#include <assert.h>
#include <setjmp.h>
#include <debug>

extern "C"
void __stack_chk_fail(void)
{
  printf("Stack overflow\n");
  abort();
}
extern "C"
int _setjmp(jmp_buf env)
{
  (void) env;
  debug2("_setjmp() called!\n");
  return 0;
}

extern "C"
void* dlopen(const char *filename, int flag)
{
  (void) flag;
  debug2("[!] dlopen called on %s\n", filename);
  return nullptr;
}
extern "C"
void* dlsym(void *handle, const char *symbol)
{
  debug2("[!] dlsym called on %p for symbol %s\n", handle, symbol);
  return nullptr;
}
extern "C"
int dlclose(void *handle)
{
  debug2("[!] dlclose called on handle %p\n", handle);
  return 0;
}

///////////////////////////////////////////////////////////////
/// printf & vararg friends
///////////////////////////////////////////////////////////////

extern "C"
int __printf_chk(int flag, const char * format, ...)
{
  debug2("printf_chk() called!\n");
  (void) flag;
  va_list args;
  va_start(args, format);
  int result = printf(format, args);
  va_end(args);
  return result;
}
extern "C"
int __fprintf_chk(FILE * stream, int flag, const char * format, ...)
{
  debug2("fprintf_chk() called! file=%p\n", stream);
  (void) flag;
  va_list args;
  va_start(args, format);
  return vfprintf(stream, format, args);
  va_end(args);
}
extern "C"
int __sprintf_chk(char * str, int flag, size_t strlen, const char * format, ...)
{
  debug2("sprintf_chk() called!\n");
  (void) flag;
  (void) strlen;
  va_list args;
  va_start(args, format);
  int result = sprintf(str, format, args);
  va_end(args);
  return result;
}
extern "C"
int __snprintf_chk(char * str, size_t maxlen, int flag, size_t strlen, const char * format, ...)
{
  debug2("snprintf_chk() called! len=%u\n", strlen);
  (void) flag;
  if (strlen < maxlen) abort();
  
  va_list args;
  va_start(args, format);
  int result = vsnprintf(str, maxlen, format, args);
  va_end(args);
  return result;
}
extern "C"
int __vfprintf_chk(FILE * fp, int flag, const char * format, va_list ap)
{
  debug2("vfprintf_chk() called!\n");
  (void) flag;
  return vfprintf(fp, format, ap);
}
extern "C"
int __vsnprintf_chk(char * s, size_t maxlen, int flag, size_t slen, const char * format, va_list args)
{
  debug2("vsnprintf_chk() called! len=%u\n", slen);
  (void) flag;
  if (slen < maxlen) abort();
  return vsnprintf(s, maxlen, format, args);
}
extern "C"
void * __rawmemchr(const void * s, int c)
{
  debug2("rawmemchr() called! addr=%p byte=%d\n", s, c);
  unsigned char* area = (unsigned char*) s;
  
  while (*area != c)
      area++;
  
  return area;
}


///////////////////////////////////////////////////////////////
/// memory & pages
///////////////////////////////////////////////////////////////

extern "C"
void * __memmove_chk(void * dest, const void * src, size_t len, size_t destlen)
{
  debug2("memmove_chk() called! dest=%p, src=%p, len=%u\n", dest, src, len);
  if (len > destlen) abort();
  return memmove(dest, src, len);
}
extern "C"
void * __memcpy_chk(void * dest, const void * src, size_t len, size_t destlen)
{
  debug2("memcpy_chk() called! dest=%p, src=%p, len=%u\n", dest, src, len);
  if (len > destlen) abort();
  return memcpy(dest, src, len);
}
extern "C"
char * __strcpy_chk(char * dest, const char * src, size_t destlen)
{
  debug2("strcpy_chk() called! dest=%p, src=%p, len=%u\n", dest, src, destlen);
  size_t len = strlen(src);
  if (len > destlen) abort();
  return strcpy(dest, src);
}

extern "C"
int mprotect(void *addr, size_t len, int prot)
{
  debug2("mprotect() called! addr=%p, len=%u\n", addr, len);
  (void) len; (void) prot;
  if ((intptr_t) addr & 4095)
      return -1;
  return 0;
}

///////////////////////////////////////////////////////////////
///  signals
///////////////////////////////////////////////////////////////
extern "C"
int sigaction(int signum, struct sigaction* act, 
                          struct sigaction* oldact)
{
  debug2("sigaction() called!\n");
  return 0;
}

#undef sigemptyset
extern "C"
int sigemptyset(sigset_t *set)
{
  debug2("sigemptyset called!\n");
  (*(set) = 0, 0);
  return 0;
}
extern "C"
void __longjmp_chk (jmp_buf env, int val)
{
  debug2("longjmp called!\n");
  longjmp(env, val);
}

extern "C"
void *tcc_mallocz(unsigned long size);

void Service::start()
{
  std::string code =
  R"VR(
    int test()
    {
      return 1;
    }
  )VR";
  
  debug2("Before tcc_new()\n");
  void* test = tcc_mallocz(64);
  printf("tcc internal malloc: %p\n", test);
  //tcc_set_lib_path(s, CONFIG_TCCDIR);
  
  TCCState* s;
  s = tcc_new();
  debug2("After tcc_new()\n");
  
  assert(s != nullptr);
  
  // I'll let someone else handle this part---
  printf ("Well, it starts.. GL!\n");
}
