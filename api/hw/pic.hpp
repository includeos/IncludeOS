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

#ifndef HW_PIC_HPP
#define HW_PIC_HPP

#include "../kernel/os.hpp"
#include "ioport.hpp"

namespace hw {

class PIC {  
public:
  static void init() noexcept;
  
  inline static void enable_irq(uint8_t irq) noexcept {
    irq_mask_ &= ~(1 << irq);
    if (irq >= 8) irq_mask_ &= ~(1 << 2);
    set_intr_mask(irq_mask_);
    INFO2("+ Enabling IRQ %i, mask: 0x%x", irq, irq_mask_);
  }
  
  inline static void disable_irq(const uint8_t irq) noexcept {
    irq_mask_ |= (1 << irq);
    if ((irq_mask_ & 0xFF00) == 0xFF00) {
      irq_mask_ |= (1 << 2);
    }
    set_intr_mask(irq_mask_);
    INFO2("- Disabling IRQ %i, mask: 0x%x", irq, irq_mask_);
  }

  inline static void eoi(const uint8_t irq) noexcept {
    
    if (irq >= 8) {
      hw::outb(slave_ctrl, eoi_);
    }
    //hw::outb(master_ctrl, specific_eoi_ | (irq & 0x0f) );*/
  }
  
  /* Returns the combined value of the cascaded PICs irq request register */
  inline static uint16_t get_irr() noexcept
  { return get_irq_reg(read_irr); }
  
  /* Returns the combined value of the cascaded PICs in-service register */
  inline static uint16_t get_isr() noexcept
  { return get_irq_reg(read_isr); }

private:
  // ICW1 bits
  static constexpr uint8_t icw1 {0x10};                // Bit 5 must always be set
  static constexpr uint8_t icw1_icw4_needed {0x1};     // Prepare for cw4 or not
  static constexpr uint8_t icw1_single_mode {0x2};     // Not set means cascade
  static constexpr uint8_t icw1_addr_interval_4 {0x4}; // Not set means interval 8
  static constexpr uint8_t icw1_level_trigered {0x8};  // Not set means egde triggred
  
  // ICW2 bits: Interrupt number for first IRQ 
  // ICW3 bits: Location of slave PIC
  // ICW4 bits: 
  static constexpr uint8_t icw4_8086_mode {0x1};       // Not set means aincient runes
  static constexpr uint8_t icw4_auto_eoi {0x2};        // Switch between auto / normal EOI
  static constexpr uint8_t icw4_buffered_mode_slave {0x08}; 
  static constexpr uint8_t icw4_buffered_mode_master {0x12};
  
  // Registers addresses
  static const uint8_t master_ctrl {0x20};
  static const uint8_t master_mask {0x21}; 
  static const uint8_t slave_ctrl  {0xA0};
  static const uint8_t slave_mask  {0xA1};

  // Master commands
  static const uint8_t master_icw1 {0x11}; // icw4 needed (bit1), bit5 always on
  static const uint8_t master_icw2 {32}; // Remap
  static const uint8_t master_icw3 {0x04}; // Location of slave
  static const uint8_t master_icw4 {0x03}; // 8086-mode (bit1), Auto-EOI (bit2)

  // Slave commands
  static const uint8_t slave_icw1 {0x11};
  static const uint8_t slave_icw2 {40};
  static const uint8_t slave_icw3 {0x02}; // Slave ID
  static const uint8_t slave_icw4 {0x01};

  /* IRQ ready next CMD read */
  static const uint8_t read_irr {0x0A};
  /* IRQ service next CMD read */
  static const uint8_t read_isr {0x0B};

  
  static constexpr uint8_t ocw2_eoi = 0x20;
  static constexpr uint8_t ocw2_specific = 0x40;

  static constexpr uint8_t eoi_ { ocw2_eoi };
  static constexpr uint8_t specific_eoi_ { ocw2_eoi | ocw2_specific };

  static uint16_t irq_mask_;
    
  inline static void set_intr_mask(uint32_t mask) noexcept {
    hw::outb(master_mask, static_cast<uint8_t>(mask));
    hw::outb(slave_mask,  static_cast<uint8_t>(mask >> 8));
  }

  /* Helper func */
  inline static uint16_t get_irq_reg(const int ocw3) noexcept {
    /*
     * OCW3 to PIC CMD to get the register values.  PIC2 is chained, and
     * represents IRQs 8-15.  PIC1 is IRQs 0-7, with 2 being the chain
     */
    hw::outb(master_ctrl, ocw3);
    hw::outb(master_ctrl, ocw3);
    return (hw::inb(slave_ctrl) << 8) | hw::inb(master_ctrl);
  }
}; //< class PIC

} //< namespace hw

#endif //< HW_PIC_HPP
