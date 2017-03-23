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

#ifndef SYS_CTYPE_H
#define SYS_CTYPE_H

#ifdef __cplusplus
extern "C" {
#endif

inline int isascii(int c) {
  return c >= 0 && c <= 0177;
}

inline int isalnum(int ch) {
  // http://en.cppreference.com/w/cpp/string/byte/isalnum
  return (ch >= 48 && ch <= 57)
    || (ch >= 65 && ch <= 90)
    || (ch >= 97 && ch <= 122);
}

#ifdef __cplusplus
}
#endif

#endif //SYS_CTYPE_H


#include_next <ctype.h>

#include <locale.h>

#ifdef __cplusplus
extern "C" {
#endif

/// isalnum, isblank, iscntrl, isdigit, isgraph, islower, isprint, ispunct, isspace, isupper, isxdigit, setlocale, uselocale
/*
inline int isalnum_l(int ch, locale_t) {
  return isalnum(ch);
}

inline int isblank_l(int ch, locale_t) {
  return isblank(ch);
}

inline int iscntrl_l(int ch, locale_t) {
  return iscntrl(ch);
}

inline int isdigit_l(int ch, locale_t) {
  return isdigit(ch);
}

inline int isgraph_l(int ch, locale_t) {
  return isgraph(ch);
}

inline int islower_l(int ch, locale_t) {
  return islower(ch);
}

inline int isprint_l(int ch, locale_t) {
  return isprint(ch);
}

inline int ispunct_l(int ch, locale_t) {
  return ispunct(ch);
}

inline int isspace_l(int ch, locale_t) {
  return isspace(ch);
}

inline int isupper_l(int ch, locale_t) {
  return isupper(ch);
}

inline int isxdigit_l(int ch, locale_t) {
  return isxdigit(ch);
}

inline int isalpha_l(int ch, locale_t) {
  return isalpha(ch);
}
*/

#ifdef __cplusplus
}
#endif
