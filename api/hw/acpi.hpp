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
#include <cstdint>

namespace hw {
  
  class ACPI {
  public:
    static void init() {
      get().discover();
    }
    
    static uint64_t time();
    
    static ACPI& get() {
      static ACPI acpi;
      return acpi;
    }
    
  private:
    void discover();
    bool checksum(const char*, size_t) const;
    void begin(const void* addr);
    
    void walk_sdts(const char* addr);
    
    
    void*    sdt_base;
    uint32_t sdt_total;
  };
  
}
