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
#include <os.hpp>
#include <kernel/elf.hpp>
#include <kprint>
#include <cmath>

/**
 * This header is for instantiating and implementing
 * missing functionality gluing libc++ to the kernel
 *
 **/

extern "C"
{
  
  int __isnan(double val)
  {
    return std::isnan(val);
  }

  int __isnanf(float val)
  {
      return std::isnan(val);
  }

  /// Linux standard base (locale)
  size_t __mbrlen (const char*, size_t, mbstate_t*)
  {
    printf("WARNING: mbrlen was called - which we don't currently support - returning bogus value\n");
    return (size_t) 1;
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
  const char* type_kind_string[] = {
    "load of", "store to", "reference binding to", "member access within",
    "member call on", "constructor call on", "downcast of", "downcast of"
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
    type_descriptor& type;
    unsigned char    log_align;
    unsigned char    type_kind;
  };
  struct function_type_mismatch {
    const source_location  src;
    const type_descriptor& type;
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
    kprintf("ubsan: %s", error);
    os::print_backtrace();
    kprintf("\n");
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
    char sbuf[1024];
    auto res = Elf::safe_resolve_symbol((void*) ptr, sbuf, sizeof(sbuf));

    const char* reason = "Type mismatch";
    const long alignment = 1 << data->log_align;

    if (alignment && (ptr & (alignment-1)) != 0) {
      reason = "Misaligned access";
    }
    else if (ptr == 0) {
      reason = "Null-pointer access";
    }
    char buffer[2048];
    snprintf(buffer, sizeof(buffer),
            "%s on ptr %p  (aligned %lu)\n"
            "ubsan: type name %s\n"
            "ubsan: symbol    %s",
            reason,
            (void*) ptr,
            alignment,
            data->type.type_name,
            res.name);
    undefined_throw(buffer);
  }
  void __ubsan_handle_function_type_mismatch(
          struct function_type_mismatch* data,
          unsigned long ptr)
  {
    print_src_location(data->src);
    char sbuf[1024];
    auto res = Elf::safe_resolve_symbol((void*) ptr, sbuf, sizeof(sbuf));

    char buffer[2048];
    snprintf(buffer, sizeof(buffer),
            "Function type mismatch on ptr %p\n"
            "ubsan: type name %s\n"
            "ubsan: function  %s",
            (void*) ptr,
            data->type.type_name,
            res.name);
    undefined_throw(buffer);
  }
  void __ubsan_handle_nonnull_arg()
  {
    undefined_throw("Non-null argument violated");
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
    os::panic("Unreachable code reached");
  }
}
