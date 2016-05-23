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
    uint32_t vector;
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
    uint32_t make_addr(uint64_t vec);
    
  private:
    PCI_Device& dev;
    intptr_t table_addr;
    intptr_t pba_addr;
    size_t   vectors;
    
    inline auto* get_entry(size_t idx)
    {
      assert(idx < vectors);
      return ((msix_entry*) table_addr) + idx;
    }
    
    intptr_t get_capbar_paddr(size_t offset);
  };
  
  class MSI
  {
    
  };
  
}

#endif
