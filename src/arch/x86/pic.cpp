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

#include <hw/ioport.hpp>
#include "pic.hpp"

namespace x86 {

  uint16_t PIC::irq_mask_ {0xFFFF};

  void PIC::init() noexcept {

    hw::outb(master_ctrl, icw1 | icw1_icw4_needed);
    hw::outb(slave_ctrl,  icw1 | icw1_icw4_needed);
    hw::outb(master_mask, icw2_irq_base_master);
    hw::outb(slave_mask,  icw2_irq_base_slave);
    hw::outb(master_mask, icw3_slave_location);
    hw::outb(slave_mask,  icw3_slave_id);

    hw::outb(master_mask, icw4_8086_mode | icw4_auto_eoi);
    hw::outb(slave_mask,  icw4_8086_mode);  // AEOI-mode only works for master

    set_intr_mask(irq_mask_);
  }
} //< namespace hw
