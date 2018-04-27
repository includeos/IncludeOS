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
#include <cstdio>
#include <stdexcept>
#include <kernel/syscalls.hpp>

/**
 * This header is for instantiating and implementing
 * missing functionality gluing libc++ to the kernel
 *
 **/

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

  using nl_catd = int;

  nl_catd catopen (const char* name, int flag)
  {
    printf("catopen: %s, flag=0x%x\n", name, flag);
    return (nl_catd) -1;
  }
  //char* catgets (nl_catd catalog_desc, int set, int message, const char *string)
  char* catgets (nl_catd, int, int, const char*)
  {
    return nullptr;
  }
  //int catclose (nl_catd catalog_desc)
  int catclose (nl_catd)
  {
    return (nl_catd) 0;
  }

  char _IO_getc()
  {
    /// NOTE: IMPLEMENT ME
    printf("_IO_getc(): returning bogus character as input from stdin\n");
    return 'x';
  }

  using ub_error = std::runtime_error;
  void undefined_throw(const char*) {
    
  }

  /// Undefined-behavior sanitizer
  void __ubsan_handle_out_of_bounds()
  {
    undefined_throw("Out-of-bounds access");
  }
  void __ubsan_handle_missing_return()
  {
    undefined_throw("Missing return");
  }

  void __ubsan_handle_add_overflow()
  {
    undefined_throw("Overflow on addition");
  }
  void __ubsan_handle_sub_overflow()
  {
    undefined_throw("Overflow on subtraction");
  }
  void __ubsan_handle_mul_overflow()
  {
    undefined_throw("Overflow on multiplication");
  }
  void __ubsan_handle_negate_overflow()
  {
    undefined_throw("Overflow on negation");
  }
  void __ubsan_handle_pointer_overflow()
  {
    undefined_throw("Pointer overflow");
  }
  void __ubsan_handle_divrem_overflow()
  {
    undefined_throw("Division remainder overflow");
  }
  void __ubsan_handle_float_cast_overflow()
  {
    undefined_throw("Float-cast overflow");
  }
  void __ubsan_handle_shift_out_of_bounds()
  {
    undefined_throw("Shift out-of-bounds");
  }

  void __ubsan_handle_type_mismatch_v1()
  {
    undefined_throw("Type mismatch");
  }
  void __ubsan_handle_function_type_mismatch()
  {
    undefined_throw("Function type mismatch");
  }
  void __ubsan_handle_load_invalid_value()
  {
    undefined_throw("Load of invalid value");
  }
  void __ubsan_handle_builtin_unreachable()
  {
    panic("Unreachable code reached");
  }
}
