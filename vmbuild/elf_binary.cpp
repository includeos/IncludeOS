#include <cstdlib>
#include <elf_binary.hpp>
#include <iostream>

extern bool verb;

const Elf32_Ehdr& Elf_binary::elf_header() const{
  Expects(data_.size() >=  (long) sizeof(Elf32_Ehdr));
  return *reinterpret_cast<Elf32_Ehdr*>(data_.data());
}

const Elf32_Phdr& Elf_binary::program_header() const{
  auto hdr = elf_header();
  Expects(data_.size() >= (long)(hdr.e_phoff + sizeof(Elf32_Phdr)));
  return *reinterpret_cast<Elf32_Phdr*>(data_.data() + hdr.e_phoff);
}

void Elf_binary::validate(){
  auto hdr = elf_header();

  if (verb){
    for(int i {0}; i < EI_NIDENT; ++i) {
      std::cout << hdr.e_ident[i];
    }

    std::cout << "\nType: "
              << ((hdr.e_type == ET_EXEC) ? " ELF Executable\n" : "Non-executable\n");
    std::cout << "Machine: ";
  }

  if (hdr.e_type != ET_EXEC)
    throw Elf_exception("Not an executable ELF binary.");

  if (verb) {
    switch (hdr.e_machine) {
    case (EM_386):
      std::cout << "Intel 80386\n";
      break;
    case (EM_X86_64):
      std::cout << "Intel x86_64\n";
      break;
    default:
      std::cout << "UNKNOWN (" << hdr.e_machine << ")\n";
      break;
    } //< switch (hdr.e_machine)

    std::cout << "Version: "                   << hdr.e_version      << '\n';
    std::cout << "Entry point: 0x"             << std::hex << hdr.e_entry << '\n';
    std::cout << "Number of program headers: " << std::dec << hdr.e_phnum        << '\n';
    std::cout << "Program header offset: "     << hdr.e_phoff        << '\n';
    std::cout << "Number of section headers: " << hdr.e_shnum        << '\n';
    std::cout << "Section header offset: "     << hdr.e_shoff        << '\n';
    std::cout << "Size of ELF-header: "        << hdr.e_ehsize << " bytes\n";
  }
}

Elf32_Addr Elf_binary::entry() {
  return elf_header().e_entry;
}

const Elf_binary::Section_headers Elf_binary::section_headers() const {
    return {reinterpret_cast<Section_header*>(data_.data() + elf_header().e_shoff), elf_header().e_shnum};
}

const Elf_binary::Section_header& Elf_binary::section_header_names() const{
  auto shstrndx = elf_header().e_shstrndx;

  if (shstrndx == SHN_LORESERVE) {
    Expects(elf_header().e_shstrndx == SHN_XINDEX);
    shstrndx = section_headers()[0].sh_link;
  }

  return section_headers()[shstrndx];
}


const Elf_binary::Span Elf_binary::section_data(const Section_header& sh) const {
  return {data_.data() + sh.sh_offset, sh.sh_size};
};

std::string Elf_binary::section_header_name(const Section_header& sh) const {

  const Elf_binary::Section_header& names_header = section_header_names();
  char* section_names = data_.data() + names_header.sh_offset;

  return section_names + sh.sh_name;
}

const Elf_binary::Section_header& Elf_binary::section_header(std::string name) const {

  for (auto& sh : section_headers()) {
    if (section_header_name(sh) == name)
      return sh;
  }

  throw Elf_exception(std::string("No section header named ") + name);
}
