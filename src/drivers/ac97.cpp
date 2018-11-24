#include "ac97.hpp"
#include <arch.hpp>
#include <kernel/events.hpp>
#include <hw/ioport.hpp>

#define AC97_NAMBAR  0x10  /* Native Audio Mixer BAR */
#define AC97_NABMBAR 0x14  /* Native Audio Bus Mastering BAR */

#define NABM_CR_RPBM   (1 << 0)  /* Run/pause bus master */
#define NABM_CR_REGRST (1 << 1)  /* Reset registers */
#define NABM_CR_IE_LVB (1 << 2)  /* Last valid buffer interrupt enable */
#define NABM_CR_IE_FE  (1 << 3)  /* FIFO error interrupt enable */
#define NABM_CR_IE_IOC (1 << 4)  /* Interrupt on completion enable */

#define PORT_NAM_RESET          0x0000
#define PORT_NAM_MASTER_VOLUME  0x0002
#define PORT_NAM_MONO_VOLUME    0x0006
#define PORT_NAM_PC_BEEP        0x000A
#define PORT_NAM_PCM_VOLUME     0x0018
#define PORT_NAM_EXT_AUDIO_ID   0x0028
#define PORT_NAM_EXT_AUDIO_STC  0x002A
#define PORT_NAM_FRONT_SPLRATE  0x002C
#define PORT_NAM_LR_SPLRATE     0x0032

#define NABM_PO_BDBAR       0x0010
#define NABM_PO_CIV         0x0014
#define NABM_PO_LVI         0x0015
#define NABM_PO_STAT        0x0016
#define NABM_PO_CTRL        0x001B
#define NABM_GLB_CTRL_STAT  0x0060

AC97::AC97(hw::PCI_Device& dev)
{
  INFO("AC97", "Intel AC97 Audio Device (rev=%#x)", dev.rev_id());
  // find and store capabilities
  dev.parse_capabilities();
  // find BARs etc.
  dev.probe_resources();
  //assert(0);
  this->m_mixer_io = dev.get_bar(0).start;
  INFO("AC97", "Mixer bar: %#hx", m_mixer_io);
  this->m_nabm_io = dev.get_bar(1).start;
  INFO("AC97", "Bus Mastering bar: %#hx", m_nabm_io);

  // AC97 bare metal IRQ
  this->m_irq = dev.get_legacy_irq();
  for (int i = 5; i < 12; i++) {
    __arch_enable_legacy_irq(i);
    Events::get().subscribe(i,
      [this, i] () {
        printf("Trigger on IRQ %u\n", i);
        this->m_irq = i;
        this->irq_handler();
      });
  }
  INFO("AC97", "Legacy IRQ: %u", this->m_irq);

  // allocate buffers
  for (size_t i = 0; i < m_desc.size(); i++) {
    const auto& buffer = m_buffers.at(i).buffer;
    m_desc.at(i).buffer = (uint32_t) (uintptr_t) buffer.data();
    m_desc.at(i).ctlv   = buffer.size() & 0xFFFF;
    m_desc.at(i).ctlv   |= (1u << 31);
  }
  uint32_t bdl = (uint32_t) (uintptr_t) m_desc.data();
  hw::outl(m_nabm_io + NABM_PO_BDBAR, bdl);

  // current index
  this->m_current_idx = 0;
  write_nabm8(NABM_PO_LVI, m_current_idx);

  // enable interrupts
  this->enable_interrupts();
  // full volume
  write_mixer16(PORT_NAM_PCM_VOLUME, 0x0);

  // start! (by unpausing)
  write_nabm8(NABM_PO_CTRL, read_nabm8(NABM_PO_CTRL) | NABM_CR_RPBM);
  //assert(dev.intx_status());
  printf("Status: %#x\n", read_nabm8(NABM_PO_STAT));
}

void AC97::irq_handler()
{
  printf("Received interrupt %u\n", this->m_irq);

}

void AC97::set_mixer(mixer_value_t knob, uint32_t)
{

}
uint32_t AC97::get_mixer(mixer_value_t knob)
{
  return 0;
}

void AC97::enable_interrupts() {
  write_nabm8(NABM_PO_CTRL, NABM_CR_IE_FE | NABM_CR_IE_IOC);
}
void AC97::disable_interrupts() {
  write_nabm8(NABM_PO_CTRL, 0x0);
}

uint8_t  AC97::read_mixer8(uint16_t offset) {
  return hw::inb(this->m_mixer_io + offset);
}
uint16_t AC97::read_mixer16(uint16_t offset) {
  return hw::inw(this->m_mixer_io + offset);
}
void AC97::write_mixer8(uint16_t offset, uint8_t value) {
  hw::outb(this->m_mixer_io + offset, value);
}
void AC97::write_mixer16(uint16_t offset, uint16_t value) {
  hw::outw(this->m_mixer_io + offset, value);
}

uint8_t  AC97::read_nabm8(uint16_t offset) {
  return hw::inb(this->m_nabm_io + offset);
}
uint16_t AC97::read_nabm16(uint16_t offset) {
  return hw::inw(this->m_nabm_io + offset);
}
void AC97::write_nabm8(uint16_t offset, uint8_t value) {
  hw::outb(this->m_nabm_io + offset, value);
}
void AC97::write_nabm16(uint16_t offset, uint16_t value) {
  hw::outw(this->m_nabm_io + offset, value);
}

#include <kernel/pci_manager.hpp>
__attribute__((constructor))
static void register_func()
{
  PCI_manager::register_snd(PCI::VENDOR_INTEL, 0x2415, &AC97::new_instance);
  PCI_manager::register_snd(PCI::VENDOR_INTEL, 0x2425, &AC97::new_instance);
  PCI_manager::register_snd(PCI::VENDOR_INTEL, 0x2445, &AC97::new_instance);
}
