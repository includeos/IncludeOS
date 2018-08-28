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

  struct source_location {
  	const char *file_name;
		struct {
			uint32_t line;
			uint32_t column;
		};
  };
  struct type_descriptor {
	   uint16_t type_kind;
	   uint16_t type_info;
	   char type_name[1];
  };

  struct out_of_bounds {
	   source_location  src;
	   type_descriptor* array_type;
	   type_descriptor* index_type;
  };
  struct overflow {
    source_location src;
  };
  struct mismatch {
    source_location  src;
    type_descriptor* type;
    unsigned char    log_align;
    unsigned char    type_kind;
  };
  struct nonnull_return {
    source_location src;
    source_location attr;
  };
  struct unreachable {
    source_location src;
  };
  using ub_error = std::runtime_error;
  void print_src_location(const source_location& src) {
    printf("ubsan: %s at line %u col %u\n",
            src.file_name, src.line, src.column);
  }
  void undefined_throw(const char* error) {
    printf("ubsan: %s", error);
    print_backtrace();
    printf("\n");
  }

  /// Undefined-behavior sanitizer
  void __ubsan_handle_out_of_bounds(struct out_of_bounds* data)
  {
    print_src_location(data->src);
    undefined_throw("Out-of-bounds access");
  }
  void __ubsan_handle_missing_return()
  {
    undefined_throw("Missing return");
  }
  void __ubsan_handle_nonnull_return(struct nonnull_return* data)
  {
    print_src_location(data->src);
    undefined_throw("Non-null return");
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
  void __ubsan_handle_divrem_overflow(struct overflow* data,
                                      unsigned long lhs,
                                      unsigned long rhs)
  {
    print_src_location(data->src);
    printf("ubsan: LHS %lu / RHS %lu\n", lhs, rhs);
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

  void __ubsan_handle_type_mismatch_v1(struct mismatch* data, unsigned long ptr)
  {
    print_src_location(data->src);
    const char* reason = "Type mismatch";
    const long alignment = 1 << data->log_align;

    if (alignment && (ptr & (alignment-1)) != 0) {
      reason = "Misaligned access";
    }
    else if (ptr == 0) {
      reason = "Null-pointer access";
    }
    char buffer[1024];
    snprintf(buffer, sizeof(buffer),
            "%s on ptr %p  (aligned %lu)",
            reason,
            (void*) ptr,
            alignment);
    undefined_throw(buffer);
  }
  void __ubsan_handle_function_type_mismatch()
  {
    undefined_throw("Function type mismatch");
  }
  void __ubsan_handle_invalid_builtin()
  {
    undefined_throw("Invalid built-in function");
  }
  void __ubsan_handle_load_invalid_value()
  {
    undefined_throw("Load of invalid value");
  }
  [[noreturn]]
  void __ubsan_handle_builtin_unreachable(struct unreachable* data)
  {
    print_src_location(data->src);
    panic("Unreachable code reached");
  }
}
