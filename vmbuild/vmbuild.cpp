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
#include <iostream>

#include <elf.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define SECT_SIZE 512
#define SECT_SIZE_ERR 99
#define DISK_SIZE_ERR 999

using namespace std;

const int offs_srvsize {2};
const int offs_srvoffs {6};

bool test {false};

const string info  {"Create a bootable disk image for IncludeOS.\n"};
const string usage {"Usage: buildvm <bootloader> <service_binary> [-test]\n"};

int main(int argc, char** argv){

  // Verify proper command usage
  if(argc < 3){
    cout << info << usage;
    exit(0);
  }
  
  const string bootloc {argv[1]};
  const string srvloc  {argv[2]};

  const string img_name {srvloc.substr(srvloc.find_last_of("/") + 1, string::npos) + ".img"};

  cout << "\nCreating VM disk image './" << img_name << "'\n";

  if((argc > 3) && (string{argv[3]} == "-test")){
    test=true;
    cout << "\n*** TEST MODE ***\n";
  }

  struct stat stat_boot;
  struct stat stat_srv;

  // Verify boot loader
  if(stat(bootloc.c_str(), &stat_boot) == -1){
    cout << "Could not open " << bootloc << " - exiting\n";
    return errno;
  }    
  
  if(stat_boot.st_size > SECT_SIZE){
    cout << "Boot sector too big! (" 
         << stat_boot.st_size << " bytes)\n";
    return SECT_SIZE_ERR;
  }

  const decltype(stat_boot.st_size) boot_sect
  { (stat_boot.st_size % SECT_SIZE) == 0 ?
    (stat_boot.st_size / SECT_SIZE) :
    (stat_boot.st_size / SECT_SIZE) + 1
  };  
  
  cout << "Size of bootloader:\t " 
       << stat_boot.st_size 
       << " bytes, => " << boot_sect
       << " sectors.\n";

  // Verify service
  if(stat(srvloc.c_str(), &stat_srv) == -1){
    cout << "Could not open " << srvloc << " - exiting.\n";
    return errno;
  }

  const decltype(stat_srv.st_size) srv_sect
  { (stat_srv.st_size % SECT_SIZE) == 0 ?
    (stat_srv.st_size / SECT_SIZE) :
    (stat_srv.st_size / SECT_SIZE) + 1
  };

  cout << "Size of service: \t" 
       << stat_srv.st_size 
       << " bytes, => " << srv_sect
       << " sectors.\n";

  const decltype(boot_sect) img_size_sect  {boot_sect + srv_sect};
  const decltype(boot_sect) img_size_bytes {(boot_sect + srv_sect) * SECT_SIZE};

  cout << "Total disk size: \t" 
       << img_size_bytes
       << " bytes, => "
       << img_size_sect
       << " sectors.\n";

  /* 
     Bochs requires old-school disk specifications. 
     sectors=cyls*heads*spt (sectors per track)
  */
  const int heads {1}, spt {63};

  const decltype(img_size_sect) cylinders
  { (img_size_sect % spt) == 0 ? 
    (img_size_sect / spt) :   // Sector count is a multiple of 63
    (img_size_sect / spt) + 1 // There's a remainder, so we add one track
  };

  const decltype(cylinders) disksize {cylinders * heads * spt * SECT_SIZE};
  
  if(disksize < img_size_bytes){
    cout << "\n---- ERROR ----\n"
         << "Image is too big for the disk!\n"
         << "Image size: " << img_size_bytes << " B\n"
         << "Disk size: "  << disksize       << " B\n";
    exit(DISK_SIZE_ERR);
  }
  
  cout << "Creating disk of size: "
       << "Cyls: "   << cylinders << "\n"
       << "Heads: "  << heads     << "\n"
       << "Sec/Tr: " << spt       << "\n"
       << "=> "      << (disksize / SECT_SIZE) << " sectors\n"
       << "=> "      << disksize  << " bytes\n";
  
  vector<char> disk (disksize);

  decltype(disk.data()) disk_head {disk.data()};
  
  // Load the boot loader into memory
  ifstream file_boot {bootloc};

  cout << "Read " << file_boot.read(disk_head, stat_boot.st_size).gcount()
       << " bytes from boot image\n";

  // Load the service into memory
  ifstream file_srv {srvloc};
  
  // Location of service code within the image
  char* srv_imgloc {disk_head + (boot_sect * SECT_SIZE)};

  cout << "Read " << file_srv.read(srv_imgloc, stat_srv.st_size).gcount()
       << " bytes from service image\n";  
   
  // ELF Header summary
  Elf32_Ehdr* elf_header {reinterpret_cast<Elf32_Ehdr*>(srv_imgloc)};

  cout << "Reading ELF headers...\n";
  cout << "Signature: ";

  for(int i {0}; i < EI_NIDENT; ++i)
    cout << elf_header->e_ident[i];

  cout << "\nType: " << (elf_header->e_type == ET_EXEC ? " ELF Executable\n" : "Non-executable\n");
  cout << "Machine: ";

  switch(elf_header->e_machine){
  case(EM_386): cout <<  "Intel 80386\n";
    break;
  case(EM_X86_64): cout << "Intel x86_64\n";
    break;
  default:
    cout << "UNKNOWN (" << elf_header->e_machine << ")\n";
    break;
  }

  cout << "Version: "                   << elf_header->e_version      << "\n";
  cout << "Entry point: 0x"             << hex << elf_header->e_entry << "\n";
  cout << "Number of program headers: " << elf_header->e_phnum        << "\n";
  cout << "Program header offset: "     << elf_header->e_phoff        << "\n";
  cout << "Number of section headers: " << elf_header->e_shnum        << "\n";
  cout << "Section header offset: "     << elf_header->e_shoff        << "\n";
  cout << "Size of ELF-header: "        << elf_header->e_ehsize << " bytes\n";
  
  cout << "\nFetching offset of section .text (the service starting point)\n";
  
  Elf32_Phdr* prog_hdr {reinterpret_cast<Elf32_Phdr*>(srv_imgloc + elf_header->e_phoff)};

  cout << "Starting at pheader 1, phys.addr: 0x" << hex << prog_hdr->p_paddr << "\n";

  decltype(elf_header->e_entry) srv_start {elf_header->e_entry};
  
  // Write OS/Service size to the bootloader
  *(reinterpret_cast<int*>(disk_head + offs_srvsize)) = srv_sect;
  *(reinterpret_cast<int*>(disk_head + offs_srvoffs)) = srv_start;
    
  int* magic_loc {reinterpret_cast<int*>(disk_head + img_size_bytes)};
      
  cout << "Applying magic signature: 0xFA7CA7"             << "\n"
       << "Data currently at location: " << img_size_bytes << "\n"
       << "Location on image: 0x" << hex << img_size_bytes << "\n"
       << "Computed memory location: " 
       << hex << (img_size_bytes - 512 + 0x8000)           << "\n";

  *magic_loc = 0xFA7CA7;

  if(test){
    cout << "\nTEST overwriting service with testdata\n";
    for(int i {0}; i < (img_size_bytes - 512); ++i)
      disk[(512 + i)] = (i % 256);
  }

  // Write the image
  FILE* image {fopen(img_name.c_str(), "w")};

  auto wrote = fwrite(disk_head, 1, disksize, image);

  cout << "Wrote "       << dec << wrote
       << " bytes => "   << (wrote / SECT_SIZE)
       << " sectors to " << img_name << "\n";

  fclose(image);
}
