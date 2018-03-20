// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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

/**
   Field medic - OS diagnostic plugins
   ____n_
   | +  |_\-;
   ;@-----@-'

   Adds extra health checks that will affect boot time and memory usage,
   but should add little or no performance overhead after service start.
**/

#include <array>

namespace medic{
  namespace diag
  {

    void init_tls();
    bool timers();
    bool elf();
    bool virtmem();
    bool tls();
    bool exceptions();
    bool stack();

    class Error : public std::runtime_error {
    public:
      using std::runtime_error::runtime_error;
      Error()
        : std::runtime_error("This is not a drill")
      {  /* TODO: Verify TLS */ }
    };


    const int bufsize = 1024;
    using Tl_bss_arr  = std::array<char, bufsize>;
    using Tl_data_arr = std::array<int, 256>;

  }
}
