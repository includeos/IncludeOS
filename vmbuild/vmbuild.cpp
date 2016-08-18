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

#include <elf_binary.hpp>
#include <elf.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <boot/multiboot.h>
#include <gsl/gsl>

#define SECT_SIZE 512
#define SECT_SIZE_ERR  666
#define DISK_SIZE_ERR  999

using namespace std;

// Location of special variables inside the bootloader
static const int bootvar_binary_size {2};
static const int bootvar_binary_location {6};

static bool test {false};

static const string info  {"Create a bootable disk image for IncludeOS.\n"};
static const string usage {"Usage: vmbuild <service_binary> [<bootloader>][-test]\n"};

class Vmbuild_error : public std::runtime_error {
  using runtime_error::runtime_error;
};


int main(int argc, char** argv) {

  // Verify proper command usage
  if (argc < 2) {
    cout << info << usage;
    exit(EXIT_FAILURE);
  }

  std::string includeos_install = std::string{getenv("HOME")} + "/IncludeOS_install";

  // Determine IncludeOS install location from environment, or set to default
  if (auto env_install = getenv("INCLUDEOS_INSTALL"))
    includeos_install = env_install;

  cout << ">>> IncludeOS install location: " << includeos_install << "\n";

  const string bootloader_path = includeos_install + "/bootloader";
  cout << ">>> Using bootloader " << bootloader_path;

  if (argc > 2)
    const string bootloader_path {argv[2]};

  const string elf_binary_path  {argv[1]};
  const string img_name {elf_binary_path.substr(elf_binary_path.find_last_of("/") + 1, string::npos) + ".img"};

  cout << "\nCreating VM disk image '" << img_name << "'\n";

  if ((argc > 3) and (string{argv[3]} == "-test")) {
    test = true;
    cout << "\n*** TEST MODE ***\n";
  }

  struct stat stat_boot;
  struct stat stat_binary;

  // Validate boot loader
  if (stat(bootloader_path.c_str(), &stat_boot) == -1) {
    cout << "Could not open " << bootloader_path << " - exiting\n";
    return errno;
  }

  if (stat_boot.st_size != SECT_SIZE) {
    cout << "Boot sector not exactly one sector in size ("
         << stat_boot.st_size << " bytes, expected: " << SECT_SIZE << ")\n";
    return SECT_SIZE_ERR;
  }
  cout << "Size of bootloader:\t" << stat_boot.st_size << '\n';

  // Validate service binary location
  if (stat(elf_binary_path.c_str(), &stat_binary) == -1) {
    cout << "Could not open " << elf_binary_path << " - exiting.\n";
    return errno;
  }

  intmax_t binary_sectors = stat_binary.st_size / SECT_SIZE;
  if (stat_binary.st_size & (SECT_SIZE-1)) binary_sectors += 1;

  cout << "Size of service: \t" << stat_binary.st_size << " bytes\n";

  const decltype(binary_sectors) img_size_sect  {1 + binary_sectors+1};
  const decltype(binary_sectors) img_size_bytes {img_size_sect * SECT_SIZE};
  Expects((img_size_bytes & (SECT_SIZE-1)) == 0);

  cout << "Total disk size: \t"
       << img_size_bytes << " bytes, => "
       << img_size_sect  << " sectors.\n";

  // Bochs requires old-school disk specifications.
  // sectors = cyls * heads * spt (sectors per track)
  /*
    const int spt = 63;
    auto disk_tracks =
    (img_size_sect % spt) == 0 ?
    (img_size_sect / spt) :    // Sector count is a multiple of 63
    (img_size_sect / spt) + 1; // There's a remainder, so we add one track

    const decltype(img_size_sect) disksize {disk_tracks * spt * SECT_SIZE};
  */

  const auto disk_size = img_size_bytes;

  cout << "Creating disk of size: " << (disk_size / SECT_SIZE) << " sectors / "
       << disk_size << " bytes\n";

  vector<char> disk (disk_size);

  auto* disk_head = disk.data();

  ifstream file_boot {bootloader_path}; //< Load the boot loader into memory

  cout << "Read " << file_boot.read(disk_head, stat_boot.st_size).gcount()
       << " bytes from boot image\n";

  ifstream file_binary {elf_binary_path}; //< Load the service into memory

  auto* binary_imgloc = disk_head + SECT_SIZE; //< Location of service code within the image

  cout << "Read " << file_binary.read(binary_imgloc, stat_binary.st_size).gcount()
       << " bytes from service image\n";

  // Validate ELF binary
  Elf_binary binary ({binary_imgloc, stat_binary.st_size});

  // Verify multiboot header
  auto& sh_multiboot = binary.section_header(".multiboot");
  multiboot_header& multiboot = *reinterpret_cast<multiboot_header*>(binary.section_data(sh_multiboot).data());


  cout << "Verifying multiboot header: \n";
  cout << "Magic value: 0x" << std::hex << multiboot.magic << "\n";
  Expects(multiboot.magic == MULTIBOOT_HEADER_MAGIC);

  cout << "Flags: " << multiboot.flags << "\n";
  cout << "Checksum: " << multiboot.checksum << "\n";
  cout << "Checksum computed: " << multiboot.checksum + multiboot.flags + multiboot.magic << "\n";

  // Verify multiboot header checksum
  Expects(multiboot.checksum + multiboot.flags + multiboot.magic == 0);

  cout << "Header addr: " << multiboot.header_addr << "\n";
  cout << "Load start: " << multiboot.load_addr << "\n";
  cout << "Load end: " << multiboot.load_end_addr << "\n";
  cout << "BSS end: " << multiboot.bss_end_addr << "\n";
  cout << "Entry: " << multiboot.entry_addr << "\n";

  // Write binary size and entry point to the bootloader
  *(reinterpret_cast<int*>(disk_head + bootvar_binary_size)) = binary_sectors;
  *(reinterpret_cast<int*>(disk_head + bootvar_binary_location)) = binary.entry();

  if (test) {
    cout << "\nTEST overwriting service with testdata\n";
    for(int i {0}; i < (img_size_bytes - 512); ++i) {
      disk[(512 + i)] = (i % 256);
    }
  } //< if (test)

  // Write the image
  auto* image = fopen(img_name.c_str(), "w");
  auto  wrote = fwrite(disk_head, 1, disk_size, image);

  cout << "Wrote "       << std::dec << wrote
       << " bytes => "   << (wrote / SECT_SIZE)
       << " sectors to " << img_name << '\n';

  fclose(image);
}
