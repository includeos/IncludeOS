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

#ifndef X86_PIC_HPP
#define X86_PIC_HPP

#include <os.hpp>
#include <hw/ioport.hpp>
#include <cassert>

namespace x86 {

  /**
     Programmable Interrupt Controller
     implementation according to Intel 8259A / 8259A-2 1988 whitepaper
  */
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

    inline static void disable() {
      set_intr_mask(0xffff);
    }

    /**
       @brief End of Interrupt. Master IRQ-lines (0-7) currently use Auto EOI,
       but the user should assume that IRQ-specific EOI's are necessary.

       @note:
       According to Intel 8259A / 8259A-2 whitepaper p. 15
       "The AEOI mode can only be used in a master 8259A and not a slave.
       8259As with a copyright date of 1985 or later will operate in the AEOI
       mode as a master or a slave"

       If I enable auto-eoi for slave, everything seems to freeze in Qemu,
       the moment I get the first network interrupt (IRQ 11).

       I'm assuming this means that I have an old chip :-)
    */
    inline static void eoi(const uint8_t irq) noexcept {

      // We're using auto-EOI-mode for IRQ < 8, IRQ >= 16 are soft-irq's
      if (irq >= 8 && irq < 16) {
        hw::outb(slave_ctrl, ocw2_specific_eoi | (irq - 8));
      }

      // If we switch off auto-EOI, we need to re-enable this
      // I'm disabling it in order to save VM-exits
      //hw::outb(master_ctrl, specific_eoi_ | irq );*/
    }

    /* Returns the combined value of the cascaded PICs irq request register */
    inline static uint16_t get_irr() noexcept
    { return get_irq_reg(ocw3_read_irr); }

    /* Returns the combined value of the cascaded PICs in-service register */
    inline static uint16_t get_isr() noexcept
    { return get_irq_reg(ocw3_read_isr); }

    inline static void set_intr_mask(uint16_t mask) noexcept {
      hw::outb(master_mask, mask & 0xFF);
      hw::outb(slave_mask,  mask >> 8);
    }

  private:
    // ICW1 bits
    static constexpr uint8_t icw1 {0x10};                // Bit 5 compulsory
    static constexpr uint8_t icw1_icw4_needed {0x1};     // Prepare for cw4 or not
    static constexpr uint8_t icw1_single_mode {0x2};     // 0: cascade
    static constexpr uint8_t icw1_addr_interval_4 {0x4}; // 0: interval 8
    static constexpr uint8_t icw1_level_trigered {0x8};  // 0: egde triggred

    // ICW2 bits: Interrupt number for first IRQ
    static constexpr uint8_t icw2_irq_base_master {32};
    static constexpr uint8_t icw2_irq_base_slave {40};

    // ICW3 bits: Location and ID of slave PIC
    static constexpr uint8_t icw3_slave_location {0x04};
    static constexpr uint8_t icw3_slave_id {0x02};

    // ICW4 bits:
    static constexpr uint8_t icw4 {0x0};                 // No bits by default
    static constexpr uint8_t icw4_8086_mode {0x1};       // 0: aincient runes
    static constexpr uint8_t icw4_auto_eoi {0x2};        // auto vs. normal EOI
    static constexpr uint8_t icw4_buffered_mode_slave {0x08};
    static constexpr uint8_t icw4_buffered_mode_master {0x12};


    // Registers addresses
    static const uint8_t master_ctrl {0x20};
    static const uint8_t master_mask {0x21};
    static const uint8_t slave_ctrl  {0xA0};
    static const uint8_t slave_mask  {0xA1};


    // Operational command words
    // OCW1: Interrupt mask
    // OCW2:
    static constexpr uint8_t ocw2 {0x0}; // No default bits
    static constexpr uint8_t ocw2_nonspecific_eoi {0x20};
    static constexpr uint8_t ocw2_specific_eoi {0x60};
    static constexpr uint8_t ocw2_rotate_on_non_specific_eoi {0xA0};
    static constexpr uint8_t ocw2_rotate_on_auto_eoi_set {0x80}; // 0x0 to clear
    static constexpr uint8_t ocw2_rotate_on_specific_eoi {0xE0};
    static constexpr uint8_t ocw2_set_priority_cmd {0xC0};
    static constexpr uint8_t ocw2_nop {0x40};

    // OCW3:
    static constexpr uint8_t ocw3 {0x08}; // Default bits
    static constexpr uint8_t ocw3_read_irr {0x02};
    static constexpr uint8_t ocw3_read_isr {0x03};
    static constexpr uint8_t ocw3_poll_cmd {0x04}; // 0 to disable
    static constexpr uint8_t ocw3_set_special_mask {0x60};
    static constexpr uint8_t ocw3_reset_special_mask {0x40};

    static uint16_t irq_mask_;

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

} //< namespace

#endif
