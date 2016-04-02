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

#include <string>
#include <debug>

/**
 * This header is for instantiating and implementing
 * missing functionality gluing libc++ to the kernel
 * 
 **/
#include <stdio.h>

extern "C"
{
  /// Linux standard base (locale)
  size_t __ctype_get_mb_cur_max(void)
  {
    return (size_t) 2; // ???
  }
  size_t __mbrlen (const char*, size_t, mbstate_t*)
  {
    printf("WARNING: mbrlen was called - which we don't currently support - returning bogus value\n");
    return (size_t) 1;
  }
  
  typedef int nl_catd;
  
  nl_catd catopen (const char* name, int flag)
  {
    printf("catopen: %s, flag=0x%x\n", name, flag);
    return (nl_catd) -1;
  }
  //char* catgets (nl_catd catalog_desc, int set, int message, const char *string)
  char* catgets (nl_catd, int, int, const char*)
  {
    return NULL;
  }
  //int catclose (nl_catd catalog_desc)
  int catclose (nl_catd)
  {
    return (nl_catd) 0;
  }
  
  char _IO_getc()
  {
    /// NOTE: IMPLEMENT ME
    debug("_IO_getc(): returning bogus character as input from stdin\n");
    return 'x';
  }
}
