
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
#ifndef HW_APIC_TIMER_HPP
#define HW_APIC_TIMER_HPP

#include <delegate>

namespace hw {
  
  struct APIC_Timer
  {
    typedef delegate<void()> handler_t;
    
    static void init();
    
    static void oneshot(uint32_t microsec);
    static void stop();
    static void set_handler(handler_t& handler);
    
    static bool     ready();
    static uint32_t long_duration();
  };

}

#endif
