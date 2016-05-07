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

#include <kernel/syscalls.hpp>
#include <hw/acpi.hpp>
#include <hw/ioport.hpp>

namespace hw {
  
  struct RSDPDescriptor {
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress;
  } __attribute__ ((packed));
  
  struct RSDPDescriptor20 {
    RSDPDescriptor rdsp10;
    
    uint32_t Length;
    uint64_t XsdtAddress;
    uint8_t  ExtendedChecksum;
    uint8_t  reserved[3];
  } __attribute__ ((packed));
  
  struct SDTHeader {
    char Signature[4];
    uint32_t Length;
    uint8_t Revision;
    uint8_t Checksum;
    char OEMID[6];
    char OEMTableID[8];
    uint32_t OEMRevision;
    uint32_t CreatorID;
    uint32_t CreatorRevision;
    
    uint32_t sigint() const {
      return *(uint32_t*) Signature;
    }
  };
  
  struct MADTRecord {
    uint8_t type;
    uint8_t length;
    uint8_t data[0];
  };
  
  struct MADTHeader {
    
    SDTHeader  hdr;
    uintptr_t  lapic_addr;
    uint32_t   flags; // 1 = dual 8259 PICs
    MADTRecord record[0];
  };
  
  struct AddressStructure
  {
    uint8_t  address_space_id; // 0 - system memory, 1 - system I/O
    uint8_t  register_bit_width;
    uint8_t  register_bit_offset;
    uint8_t  reserved;
    uint64_t address;
  };
  
  struct pci_vendor_t
  {
    uint16_t    ven_id;
    const char* ven_name;
  };
  
  struct HPET
  {
    uint8_t hardware_rev_id;
    uint8_t comparator_count :5;
    uint8_t counter_size     :1;
    uint8_t reserved         :1;
    uint8_t legacy_replacem  :1;
    pci_vendor_t pci_vendor_id;
    AddressStructure address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
  } __attribute__((packed));
  
  uint64_t ACPI::time() {
    return 0;
  }
  
  void ACPI::begin(const void* addr) {
    
    auto* rdsp = (RSDPDescriptor20*) addr;
    printf("--- ACPI ---\nOEM: %.*s Rev. %u\n", 
        6, rdsp->rdsp10.OEMID, rdsp->rdsp10.Revision);
    
    auto* rsdt = (SDTHeader*) rdsp->rdsp10.RsdtAddress;
    // verify Root SDT
    if (!checksum((char*) rsdt, rsdt->Length)) {
      printf("ACPI: SDT failed checksum!");
      panic("SDT checksum failed");
    }
    
    // walk through system description table headers
    // remember the interesting ones, and count CPUs
    walk_sdts((char*) rsdt);
  }
  
  constexpr uint32_t bake(char a, char b , char c, char d) {
    return a | (b << 8) | (c << 16) | (d << 24);
  }
  
  void ACPI::walk_sdts(const char* addr)
  {
    // find total number of SDTs
    auto* rsdt = (SDTHeader*) addr;
    int  total = (rsdt->Length - sizeof(SDTHeader)) / 4;
    // go past rsdt
    addr += sizeof(SDTHeader);
    
    // parse all tables
    constexpr uint32_t APIC_t = bake('A', 'P', 'I', 'C');
    constexpr uint32_t HPET_t = bake('H', 'P', 'E', 'T');
    
    while (total) {
      // convert *addr to SDT-address
      auto sdt_ptr = *(intptr_t*) addr;
      // create SDT pointer
      auto* sdt = (SDTHeader*) sdt_ptr;
      // find out which SDT it is
      switch (sdt->sigint()) {
      case APIC_t:
        printf("APIC found: P=%p L=%u\n", sdt, sdt->Length);
        walk_madt((char*) sdt);
        break;
      case HPET_t:
        printf("HPET found: P=%p L=%u\n", sdt, sdt->Length);
        this->hpet_base = sdt_ptr + sizeof(SDTHeader);
        break;
      default:
        printf("Signature: %.*s (u=%u)\n", 4, sdt->Signature, sdt->sigint());
      }
      
      addr += 4; total--;
    }
    printf("Finished walking SDTs\n");
  }
  
  void ACPI::walk_madt(const char* addr)
  {
    auto* hdr = (MADTHeader*) addr;
    printf("--> LAPIC addr: 0x%x  flags: 0x%x\n", 
        hdr->lapic_addr, hdr->flags);
    this->apic_base = hdr->lapic_addr;
    
    // the length remaining after MADT header
    int len = hdr->hdr.Length - sizeof(MADTHeader);
    // start walking
    const char* ptr = (char*) hdr->record;
    
    while (len) {
      auto* rec = (MADTRecord*) ptr;
      switch (rec->type) {
      case 0:
        {
          auto& lapic = *(LAPIC*) rec;
          lapics.push_back(lapic);
          printf("  | --> LAPIC: cpu=%u id=%u flags=0x%x\n", 
              lapic.cpu, lapic.id, lapic.flags);
        }
        break;
      case 1:
        break;
      }
      // decrease length as we go
      len -= rec->length;
      // go to next entry
      ptr += rec->length;
    }
  }
  
  bool ACPI::checksum(const char* addr, size_t size) const {
    
    const char* end = addr + size;
    uint8_t sum = 0;
    while (addr < end) {
      sum += *addr; addr++;
    }
    return sum == 0;
  }
  
  void ACPI::discover() {
    // "RSD PTR "
    const uint64_t sign = 0x2052545020445352;
    
    // guess at QEMU location of RDSP
    const auto* guess = (char*) 0xf6450;
    if (*(uint64_t*) guess == sign) {
      if (checksum(guess, sizeof(RSDPDescriptor))) {
        printf("Found ACPI located at QEMU-guess (%p)\n", guess);
        begin(guess);
        return;
      }
    }
    
    // search in BIOS area (below 1mb)
    const auto* addr = (char*) 0x000e0000;
    const auto* end  = (char*) 0x000fffff;
    printf("Looking for ACPI at %p\n", addr);
    
    while (addr < end) {
      
      if (*(uint64_t*) addr == sign) {
        // verify checksum of RDSP
        if (checksum(addr, sizeof(RSDPDescriptor))) {
          printf("Found ACPI located at %p\n", addr);
          begin(addr);
          return;
        }
        else {
         printf("Bad RDSP checksum at %p\n", addr); 
        }
      }
      addr++;
    }
    
    panic("ACPI lookup failed\n");
  }
  
}
