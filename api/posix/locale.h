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

#ifndef SYS_LOCALE_H
#define SYS_LOCALE_H

#define _LIBCPP_SUPPORT_NEWLIB_XLOCALE_H
#define _LIBCPP_SUPPORT_XLOCALE_NOP_LOCALE_MGMT_H

#ifdef __cplusplus
extern "C" {
#endif

/*
typedef void* locale_t;

static inline locale_t duplocale(locale_t) {
  return (locale_t) 0;
}
static inline void freelocale(locale_t) {
  //
}
static inline locale_t newlocale(int, const char *, locale_t)
{
  return (locale_t) 0;
}
static inline char* setlocale(int, const char*)
{
  return nullptr;
}
static inline locale_t uselocale(locale_t newloc) {
  return (locale_t) 0;
}
*/

#ifdef __cplusplus
}
#endif

#endif // SYS_LOCALE_H

#include_next <locale.h>
