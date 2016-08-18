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

#include <elf.h>
#include <gsl/gsl>
#include <stdexcept>

class Elf_binary {

public:

  using Span = gsl::span<char>;
  using Section_header = Elf32_Shdr;
  using Section_headers = gsl::span<Section_header>;

  Elf_binary(Span data)
    : data_{data}
  {
    validate();
  }

  const Elf32_Ehdr& elf_header() const;
  const Elf32_Phdr& program_header() const;
  const Elf32_Shdr& section_header() const;

  /** Make sure this is a valid ELF binary **/
  void validate();

  /** Program entry point **/
  Elf32_Addr entry();

  /** Get the span of seciton headers **/
  const Section_headers section_headers() const;

  /** Get a section header by name **/
  const Section_header& section_header(std::string name) const;

  const Section_header& section_header_names() const;

  std::string section_header_name(const Section_header& sh) const;
  const Span section_data(const Section_header& sh) const;

private:
  Span data_;

};

class Elf_exception : public std::runtime_error {
  using runtime_error::runtime_error;
};

#endif
