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

  static inline int isascii(int c){
    return c >= 0 && c <= 0177;
  }

  static inline int isalnum(int ch){
    // http://en.cppreference.com/w/cpp/string/byte/isalnum
    return (ch >= 48 && ch <= 57)
      or (ch >= 65 && ch <= 90)
      or (ch >= 97 && ch <= 122);
  }


#ifdef __cplusplus
}
#endif

#endif //SYS_CTYPE_H


#include_next <ctype.h>
