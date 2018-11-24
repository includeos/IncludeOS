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

#include "smbios.hpp"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <info>
#include <os.hpp>
#include <kernel.hpp>

namespace x86
{
  arch_system_info_t SMBIOS::sysinfo;

  struct EntryPoint
  {
 	  char EntryPointString[4];    // "_SM_"
 	  uint8_t  checksum;           // This value summed with all the values of the table, should be 0 (overflow)
 	  uint8_t  length;             // Length of the Entry Point Table. Since version 2.1 of SMBIOS, this is 0x1F
 	  uint8_t  major;              // Major Version of SMBIOS
 	  uint8_t  minor;              // Minor Version of SMBIOS
 	  uint16_t MaxStructureSize;   // Maximum size of a SMBIOS Structure (we will se later)
 	  uint8_t  EntryPointRevision;   //...
 	  char     FormattedArea[5];     //...
 	  char     EntryPointString2[5]; // "_DMI_"
 	  uint8_t  Checksum2;           // Checksum for values from EntryPointString2 to the end of table
 	  uint16_t TableLength;         // Length of the Table containing all the structures
 	  uint32_t TableAddress;	      // Address of the Table
 	  uint16_t num_structures;      // Number of structures in the table
 	  uint8_t  BCDRevision;         //Unused
  };

  struct Header
  {
    uint8_t  type;
    uint8_t  length;
    uint16_t handle;

    const char data[0];

    const char* strings() const
    {
      return &this->data[this->length - sizeof(Header)];
    }

    const char* get_string(int idx) const
    {
      auto* str = strings();
      while (idx-- > 0)
      {
        str += strnlen(str, 64) + 1;
      }
      return str;
    }

    const Header* next() const
    {
      auto* str = strings();
      while (true)
      {
        int len = strnlen(str, 64);
        str += len + 1;
        if (len == 0) return (const Header*) str;
      }
    }
  } __attribute__((packed));

  struct PhysMemArray : public Header
  {
    struct bits_t
    {
      uint8_t location;
      uint8_t use;
      uint8_t error_corr_mtd;
      uint32_t capacity32;
      uint16_t mem_err_info_handle;
      uint16_t num_slots;
      uint64_t capacity64;

    } __attribute__((packed));

    const bits_t& info() const noexcept {
      return *(bits_t*) &data[0];
    }

    uintptr_t capacity() const noexcept {
      const auto& inf = info();
      return (inf.capacity32 == 0x80000000)
        ? inf.capacity64 : inf.capacity32 * 1024;
    }
  } __attribute__((packed));

  void SMBIOS::parse(const char* mem)
  {
    auto* table = (const EntryPoint*) mem;
    INFO("SMBIOS", "Version %u.%u", table->major, table->minor);

    const int structs = table->num_structures;
    auto* hdr = (const Header*) (uintptr_t) table->TableAddress;
    for (int i = 0; i < structs; i++)
    {
      //printf("Table: %u\n", hdr->type);
      switch (hdr->type)
      {
      case 0: // BIOS
        INFO2("BIOS vendor: %s", hdr->get_string(hdr->data[0]));
        INFO2("BIOS version: %s", hdr->get_string(hdr->data[1]));
        break;
      case 1:
        INFO2("Manufacturer: %s", hdr->get_string(hdr->data[0]));
        INFO2("Product name: %s", hdr->get_string(hdr->data[1]));
        {
          // UUID starts at offset 0x4 after header
          char uuid[33];
          for (int i = 0; i < 16; i++) {
            sprintf(&uuid[i*2], "%02hhx", hdr->data[0x4 + i]);
          }
          sysinfo.uuid = std::string(uuid, 32);
          INFO2("System UUID: %s", sysinfo.uuid.c_str());
        }
        break;
      case 16:
        {
          const auto* array = (PhysMemArray*) hdr;
          sysinfo.physical_memory = array->capacity();
          INFO2("Physical memory array with %lu MB capacity",
                sysinfo.physical_memory / (1024*1024));
        }
        break;
      }

      hdr = hdr->next();
      if (hdr->type == 0) break;
    }

    // salvage operation for when no memory array found
    if (sysinfo.physical_memory == 0) {
      sysinfo.physical_memory = kernel::memory_end()+1;
    }
  }

  static uint8_t checksum(const char* addr, int length)
  {
    uint8_t sum = 0;
    for (int i = 0; i < length; i++) sum += addr[i];
    return sum;
  }

  void SMBIOS::init()
  {
    auto* mem = (const char*) 0xF0000;
    while (mem < (const char*) 0x100000)
    {
      if (strncmp(mem, "_SM_", 4) == 0)
      {
        if (checksum(mem, mem[5]) == 0)
        {
          //printf("Found SMBIOS entry @ %p\n", mem);
          SMBIOS::parse(mem);
          return;
        }
      }
      mem += 16;
    }
    assert(0 && "Failed to find SMBIOS headers\n");
  }

}

const arch_system_info_t& __arch_system_info() noexcept
{
  return x86::SMBIOS::system_info();
}
