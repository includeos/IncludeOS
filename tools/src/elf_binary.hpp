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

#ifndef ELF_BINARY_HPP
#define ELF_BINARY_HPP

#include "elf.h"
#include <gsl/gsl>
#include <stdexcept>


struct Elf32 {
  using Ehdr = Elf32_Ehdr;
  using Shdr = Elf32_Shdr;
  using Phdr = Elf32_Phdr;
  using Addr = Elf32_Addr;
};

struct Elf64{
  using Ehdr = Elf64_Ehdr;
  using Shdr = Elf64_Shdr;
  using Phdr = Elf64_Phdr;
  using Addr = Elf64_Addr;
};

template <typename Arch>
class Elf_binary {

public:

  using Span = gsl::span<char>;
  using Elf_header = typename Arch::Ehdr;
  using Section_header = typename Arch::Shdr;
  using Section_headers = gsl::span<Section_header>;
  using Program_header = typename Arch::Phdr;
  using Program_headers = gsl::span<Program_header>;
  using Addr = typename Arch::Addr;

  Elf_binary(Span data)
    : data_{data}
  {
    Expects(is_ELF());
  }

  const Elf_header& elf_header() const;
  const Program_headers program_headers() const;
  const Section_header& section_header() const;

  /** Make sure this is a valid ELF binary. Throws if not. **/
  void validate();

  bool is_ELF();
  bool is_executable();
  bool is_bootable();

  /** Print human-readable summary */
  void print_summary();

  /** Program entry point **/
  Addr entry();

  /** Program headers marked loadable */
  std::vector<const Program_header*> loadable_segments();

  /** Get the span of seciton headers **/
  const Section_headers section_headers() const;

  /** Get a section header by name **/
  const Section_header& section_header(std::string name) const;

  const Section_header& section_header_names() const;

  std::string section_header_name(const Section_header& sh) const;
  const Span section_data(const Section_header& sh) const;

private:
  Span data_;
  Program_header& program_header() const;

};

class Elf_exception : public std::runtime_error {
  using runtime_error::runtime_error;
};

// Implementation
#include "elf_binary.inc"

#endif
