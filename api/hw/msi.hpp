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

#pragma once
#ifndef HW_MSI_HPP
#define HW_MSI_HPP

#include <cstdint>
#include <cstddef>
#include <cassert>

namespace hw {
  
  class PCI_Device;
  
  struct msix_entry
  {
    uint32_t vector_ctl;
    uint32_t msg_data;
    uint32_t msg_upper_addr;
    uint32_t msg_addr;
  };
  struct msix_pba
  {
    // qword pending bits 0-63 etc.
    uint64_t pending[0];
  };
  
  struct msix_t
  {
    msix_t(PCI_Device&);
    
    // initialize msi-x tables for device
    void init(PCI_Device&);
    void mask_entry(size_t);
    void unmask_entry(size_t);
    void zero_entry(size_t);
    // enable one (cpu, vector) entry for this device
    uint16_t setup_vector(uint8_t cpu, uint8_t vector);
    
    uint16_t vectors() const noexcept {
      return vector_cnt;
    }
    
    void reset_pba_bit(size_t vec);
    
  private:
    PCI_Device& dev;
    uintptr_t table_addr;
    uintptr_t pba_addr;
    uint16_t  vector_cnt;
    
    inline auto* get_entry(size_t idx)
    {
      assert(idx < vectors());
      return ((msix_entry*) table_addr) + idx;
    }
    inline uintptr_t get_entry(size_t idx, size_t offs);
    uintptr_t get_pba(size_t offs);
    
    // get physical address of BAR
    uintptr_t get_bar_paddr(size_t offset);
  };
  
  class MSI
  {
    
  };
  
}

#endif
