// -*-C++-*-
// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

#include <iostream>

template <typename Arch>
const typename Arch::Ehdr& Elf_binary<Arch>::elf_header() const{
  Expects(data_.size() >=  (long) sizeof(typename Arch::Ehdr));
  return *reinterpret_cast<Elf_header*>(data_.data());
}

template <typename Arch>
typename Arch::Phdr& Elf_binary<Arch>::program_header() const{
  auto hdr = elf_header();
  Expects(data_.size() >= (long)(hdr.e_phoff + sizeof(typename Arch::Phdr)));
  return *reinterpret_cast<Program_header*>(data_.data() + hdr.e_phoff);
}


template <typename Arch>
const typename Elf_binary<Arch>::Program_headers Elf_binary<Arch>::program_headers() const {
  return {reinterpret_cast<Program_header*>(&program_header()), elf_header().e_phnum};
}


template <typename Arch>
void Elf_binary<Arch>::print_summary() {

  auto hdr = elf_header();

  for(int i {0}; i < EI_NIDENT; ++i) {
    std::cout << hdr.e_ident[i];
  }

  std::cout << "\nType: "
            << ((hdr.e_type == ET_EXEC) ? " ELF Executable\n" : "Non-executable\n");
  std::cout << "Machine: ";
  switch (hdr.e_machine) {
  case (EM_386):
    std::cout << "Intel 80386\n";
    break;
  case (EM_X86_64):
    std::cout << "Intel x86_64\n";
    break;
  default:
    std::cout << "Other (" << hdr.e_machine << ")\n";
    break;
  }

  std::cout << "Version: "                   << hdr.e_version      << '\n';
  std::cout << "Entry point: 0x"             << std::hex << hdr.e_entry << '\n';
  std::cout << "Number of program headers: " << std::dec << hdr.e_phnum        << '\n';
  std::cout << "Program header offset: "     << hdr.e_phoff        << '\n';
  std::cout << "Number of section headers: " << hdr.e_shnum        << '\n';
  std::cout << "Section header offset: "     << hdr.e_shoff        << '\n';
  std::cout << "Size of ELF-header: "        << hdr.e_ehsize << " bytes\n";
}


template <typename Arch>
bool Elf_binary<Arch>::is_executable(){
  return elf_header().e_type == ET_EXEC;
}

template <typename Arch>
bool Elf_binary<Arch>::is_bootable(){

  bool has_multiboot = false;

  try {
    section_header(".multiboot");
    has_multiboot = true;
  }catch(...) {}

  return is_executable() and has_multiboot;
}

template <typename Arch>
bool Elf_binary<Arch>::is_ELF(){
  Expects(data_.size() > 4);
  return
    data_[EI_MAG0] == ELFMAG0 and
    data_[EI_MAG1] == ELFMAG1 and
    data_[EI_MAG2] == ELFMAG2 and
    data_[EI_MAG3] == ELFMAG3;

}


template <typename Arch>
void Elf_binary<Arch>::validate(){

  if (not is_ELF() or not is_bootable() or not is_executable())
    throw Elf_exception("Not a bootable and executable ELF binary.");

}

template <typename Arch>
typename Arch::Addr Elf_binary<Arch>::entry() {
  return elf_header().e_entry;
}


template <typename Arch>
std::vector<const typename Arch::Phdr*> Elf_binary<Arch>::loadable_segments() {
  std::vector<const Program_header*> hdrs;
  for (auto& phdr : program_headers()) {
    if (phdr.p_type == PT_LOAD)
      hdrs.push_back(&phdr);
  }
  return hdrs;
};


template <typename Arch>
const typename Elf_binary<Arch>::Section_headers Elf_binary<Arch>::section_headers() const {
    return {reinterpret_cast<Section_header*>(data_.data() + elf_header().e_shoff),
        elf_header().e_shnum};
}

template <typename Arch>
const typename Elf_binary<Arch>::Section_header& Elf_binary<Arch>::section_header_names() const{
  auto shstrndx = elf_header().e_shstrndx;

  if (shstrndx == SHN_LORESERVE) {
    Expects(elf_header().e_shstrndx == SHN_XINDEX);
    shstrndx = section_headers()[0].sh_link;
  }

  return section_headers()[shstrndx];
}


template <typename Arch>
const typename Elf_binary<Arch>::Span Elf_binary<Arch>::section_data(const Section_header& sh) const {
  return {data_.data() + sh.sh_offset, static_cast<std::ptrdiff_t>(sh.sh_size)};
};

template <typename Arch>
std::string Elf_binary<Arch>::section_header_name(const Section_header& sh) const {

  const Elf_binary<Arch>::Section_header& names_header = section_header_names();
  char* section_names = data_.data() + names_header.sh_offset;

  return section_names + sh.sh_name;
}

template <typename Arch>
const typename Elf_binary<Arch>::Section_header& Elf_binary<Arch>::section_header(std::string name) const {

  for (auto& sh : section_headers()) {
    if (section_header_name(sh) == name)
      return sh;
  }

  throw Elf_exception(std::string("No section header named ") + name);
}
