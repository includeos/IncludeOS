#include <hw/pic.hpp>

uint16_t PIC::irq_mask_ = 0xFFFB;

void PIC::init(){
  OS::outb(master_ctrl, master_icw1);
  OS::outb(slave_ctrl, slave_icw1);
  OS::outb(master_mask, master_icw2);
  OS::outb(slave_mask, slave_icw2);
  OS::outb(master_mask, master_icw3);
  OS::outb(slave_mask, slave_icw3);
  OS::outb(master_mask, master_icw4);
  OS::outb(slave_mask, slave_icw4);

  set_intr_mask(irq_mask_);

}
