
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
