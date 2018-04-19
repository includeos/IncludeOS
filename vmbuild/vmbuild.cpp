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

#include <vector>
#include <fstream>
#include <cstring>
#include <iostream>

#include <errno.h>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <cassert>
#include <elf.h>

#include "../api/boot/multiboot.h"

#define GSL_THROW_ON_CONTRACT_VIOLATION
#include "../api/util/elf_binary.hpp"

#define SECT_SIZE 512
#define SECT_SIZE_ERR  666
#define DISK_SIZE_ERR  999

bool verb = false;

#define INFO_(FROM, TEXT, ...) if (verb) fprintf(stderr, "%13s ] " TEXT "\n", "[ " FROM, ##__VA_ARGS__)
#define INFO(X,...) INFO_("Vmbuild", X, ##__VA_ARGS__)
#define WARN(X,...) fprintf(stderr, "[ vmbuild ] Warning: " X "\n", ##__VA_ARGS__)
#define ERROR(X,...) fprintf(stderr, "[ vmbuild ] Error: " X "\n", ##__VA_ARGS__); std::terminate()


// Special variables inside the bootloader
struct bootvars {
  const uint32_t __first_instruction;
  uint32_t size;
  uint32_t entry;
  uint32_t load_addr;
};

static bool test {false};

const std::string info  {"Create a bootable disk image for IncludeOS.\n"};
const std::string usage {"Usage: vmbuild <service_binary> [<bootloader>][-test]\n"};

std::string includeos_install = "/usr/local/includeos/";

class Vmbuild_error : public std::runtime_error {
  using runtime_error::runtime_error;
};

std::string get_bootloader_path(int argc, char** argv) {

  if (argc == 2) {
    // Determine IncludeOS install location from environment, or set to default
    std::string arch = "x86_64";
    auto env_arch = getenv("ARCH");

    if (env_arch)
      arch = std::string(env_arch);

    std::string includeos_install = "/usr/local/includeos/" + arch;

    if (auto env_prefix = getenv("INCLUDEOS_PREFIX"))
      includeos_install = std::string{env_prefix} + "/includeos/" + arch;

    return includeos_install + "/boot/bootloader";
  } else {
    return argv[2];
  }
}

int main(int argc, char** argv)
{
  // Verify proper command usage
  if (argc < 2) {
    std::cout << info << usage;
    exit(EXIT_FAILURE);
  }

  // Set verbose from environment
  const char* env_verb = getenv("VERBOSE");
  if (env_verb && strlen(env_verb) > 0)
      verb = true;

  const std::string bootloader_path = get_bootloader_path(argc, argv);

  if (argc > 2)
    const std::string bootloader_path {argv[2]};

  INFO("Using bootloader %s" , bootloader_path.c_str());

  const std::string elf_binary_path  {argv[1]};
  const std::string img_name {elf_binary_path.substr(elf_binary_path.find_last_of("/") + 1, std::string::npos) + ".img"};

  INFO("Creating image '%s'" , img_name.c_str());

  if (argc > 3) {
    if (std::string{argv[3]} == "-test") {
      test = true;
      verb = true;
    } else if (std::string{argv[3]} == "-v"){
      verb = true;
    }
  }

  struct stat stat_boot;
  struct stat stat_binary;

  // Validate boot loader
  if (stat(bootloader_path.c_str(), &stat_boot) == -1) {
    INFO("Could not open %s, exiting\n" , bootloader_path.c_str());
    return errno;
  }

  if (stat_boot.st_size != SECT_SIZE) {
    INFO("Boot sector not exactly one sector in size (%ld bytes, expected %i)",
         stat_boot.st_size, SECT_SIZE);
    return SECT_SIZE_ERR;
  }

  INFO("Size of bootloader: %ld\t" , stat_boot.st_size);

  // Validate service binary location
  if (stat(elf_binary_path.c_str(), &stat_binary) == -1) {
    ERROR("vmbuild: Could not open '%s'\n" , elf_binary_path.c_str());
    return errno;
  }

  intmax_t binary_sectors = stat_binary.st_size / SECT_SIZE;
  if (stat_binary.st_size & (SECT_SIZE-1)) binary_sectors += 1;

  INFO("Size of service: \t%ld bytes" , stat_binary.st_size);

  const decltype(binary_sectors) img_size_sect  {1 + binary_sectors};
  const decltype(binary_sectors) img_size_bytes {img_size_sect * SECT_SIZE};
  assert((img_size_bytes & (SECT_SIZE-1)) == 0);

  INFO("Total disk size: \t%ld bytes, => %ld sectors",
       img_size_bytes, img_size_sect);

  const auto disk_size = img_size_bytes;

  INFO("Creating disk of size %ld sectors / %ld bytes" ,
       (disk_size / SECT_SIZE), disk_size);

  std::vector<char> disk (disk_size);

  auto* disk_head = disk.data();

  std::ifstream file_boot {bootloader_path}; //< Load the boot loader into memory

  auto read_bytes = file_boot.read(disk_head, stat_boot.st_size).gcount();
  INFO("Read %ld bytes from boot image", read_bytes);

  std::ifstream file_binary {elf_binary_path}; //< Load the service into memory

  auto* binary_imgloc = disk_head + SECT_SIZE; //< Location of service code within the image

  read_bytes = file_binary.read(binary_imgloc, stat_binary.st_size).gcount();
  INFO("Read %ld bytes from service image" , read_bytes);

  // only accept ELF binaries
  if (not (binary_imgloc[EI_MAG0] == ELFMAG0
           && binary_imgloc[EI_MAG1] == ELFMAG1
           && binary_imgloc[EI_MAG2] == ELFMAG2
           && binary_imgloc[EI_MAG3] == ELFMAG3))
  {
    ERROR("Not ELF binary");
  }

  // Required bootloader info
  uint32_t srv_entry{};
  uint32_t srv_load_addr{};
  uint32_t binary_load_offs{};

  multiboot_header* multiboot_hdr = nullptr;

  // 32-bit ELF
  if (binary_imgloc[EI_CLASS] == ELFCLASS32)
   {
    Elf_binary<Elf32> binary ({binary_imgloc, stat_binary.st_size});
    binary.validate();
    srv_entry = binary.entry();

    INFO("Found 32-bit ELF with entry at 0x%x", srv_entry);

    auto loadable = binary.loadable_segments();
    if (loadable.size() > 1) {
      WARN("found %zu loadable segments. Loading as one.",loadable.size());
    }
    srv_load_addr = loadable[0]->p_paddr;
    binary_load_offs = loadable[0]->p_offset;

    auto& sh_multiboot = binary.section_header(".multiboot");
    multiboot_hdr = reinterpret_cast<multiboot_header*>(binary.section_data(sh_multiboot).data());

  }

  // 64-bit ELF
  else if (binary_imgloc[EI_CLASS] == ELFCLASS64)
  {
    Elf_binary<Elf64> binary ({binary_imgloc, stat_binary.st_size});
    binary.validate();
    srv_entry = binary.entry();

    INFO("Found 64-bit ELF with entry at 0x%x", srv_entry);

    auto loadable = binary.loadable_segments();
    // Expects(loadable.size() == 1);
    // TODO: Handle multiple loadable segments properly
    srv_load_addr = loadable[0]->p_paddr;
    binary_load_offs = loadable[0]->p_offset;

    auto& sh_multiboot = binary.section_header(".multiboot");
    multiboot_hdr = reinterpret_cast<multiboot_header*>(binary.section_data(sh_multiboot).data());
  }

  // Unknown ELF format
  else
  {
    ERROR("Unknown ELF format");
  }


  INFO("Verifying multiboot header:");
  INFO("Magic value: 0x%x" , multiboot_hdr->magic);
  if (multiboot_hdr->magic != MULTIBOOT_HEADER_MAGIC) {
    ERROR("Multiboot magic mismatch: 0x%08x vs %#x",
          multiboot_hdr->magic, MULTIBOOT_HEADER_MAGIC);
  }


  INFO("Flags: 0x%x" , multiboot_hdr->flags);
  INFO("Checksum: 0x%x" , multiboot_hdr->checksum);
  INFO("Checksum computed: 0x%x", multiboot_hdr->checksum + multiboot_hdr->flags + multiboot_hdr->magic);

  // Verify multiboot header checksum
  assert(multiboot_hdr->checksum + multiboot_hdr->flags + multiboot_hdr->magic == 0);

  INFO("Header addr: 0x%x" , multiboot_hdr->header_addr);
  INFO("Load start: 0x%x" , multiboot_hdr->load_addr);
  INFO("Load end: 0x%x" , multiboot_hdr->load_end_addr);
  INFO("BSS end: 0x%x" , multiboot_hdr->bss_end_addr);
  INFO("Entry: 0x%x" , multiboot_hdr->entry_addr);

  assert(multiboot_hdr->entry_addr == srv_entry);

  // Load binary starting at first loadable segmento
  uint32_t srv_size = binary_sectors;

  srv_load_addr -= binary_load_offs;

  // Write binary size and entry point to the bootloader
  bootvars* boot = reinterpret_cast<bootvars*>(disk_head);

  boot->size       = srv_size;
  boot->entry      = srv_entry;
  boot->load_addr  = srv_load_addr;

  INFO("srv_size: %i", srv_size);
  INFO("srv_entry: 0x%x", srv_entry);
  INFO("srv_load: 0x%x", srv_load_addr);

  if (test) {
    INFO("\nTEST overwriting service with testdata");
    for(int i {0}; i < (img_size_bytes - 512); ++i) {
      disk[(512 + i)] = (i % 256);
    }
  } //< if (test)

  // Write the image
  auto* image = fopen(img_name.c_str(), "w");
  auto  wrote = fwrite(disk_head, 1, disk_size, image);

  INFO("Wrote %ld bytes => %ld sectors to '%s'",
       wrote, (wrote / SECT_SIZE), img_name.c_str());

  fclose(image);
}
