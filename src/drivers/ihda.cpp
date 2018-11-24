#include "ihda.hpp"
#include <arch.hpp>
#include <kernel/events.hpp>
#include <immintrin.h>
#include <info>

#define REG_GCTL      0x08
#define REG_STATESTS  0x0E
static const uint16_t RST_BIT = 1 << 15;
#define RING_LBASE(r) (r.reg + 0x0)
#define RING_UBASE(r) (r.reg + 0x4)
#define RING_WP(r)    (r.reg + 0x8)
#define RING_RP(r)    (r.reg + 0xA)
#define RING_CTL(r)   (r.reg + 0xC)
#define RING_STAT(r)  (r.reg + 0xD)
#define RING_SIZE(r)  (r.reg + 0xE)
#define REG_IMM_CMD   0x60
#define REG_IMM_RESP  0x64
#define REG_IMM_STAT  0x68

static const uint32_t VERB_GET = 0xF0000;

IHDA::IHDA(hw::PCI_Device& dev)
{
  INFO("IntelHDA", "+--[ Intel HD Audio Device (rev=%#x) ]", dev.rev_id());
  // find and store capabilities
  dev.parse_capabilities();
  // find BARs etc.
  dev.probe_resources();
  // MSI config
  if (dev.msix_cap() || dev.msi_cap())
  {
    dev.init_msix();
    assert(dev.has_msix());

    if (dev.msi_cap()) {
      this->m_irq = dev.get_legacy_irq();
    }
    Events::get().subscribe(this->m_irq, {this, &IHDA::event_handler});
  }
  else {
    assert(0 && "Legacy INT#x not supported");
  }
  INFO2("|  |- IRQ: %u", this->m_irq);
  // MMIO BAR
  this->m_hdbar = dev.get_bar(0).start;
  INFO2("|  |- MMIO address: %p", (void*) this->m_hdbar);

  const uint8_t major = read<uint8_t>(0x3);
  const uint8_t minor = read<uint8_t>(0x2);
  INFO2("|  |- Version: %d.%d", major, minor);

  this->reset();
  this->detect_codecs();
  INFO("IntelHDA", "|  +--[ Codecs: %zu ]", m_codecs.size());
  m_corb.reg = 0x40;
  m_rirb.reg = 0x50;
  this->setup_ring(m_corb, true);
  this->setup_ring(m_rirb, false);

  for (auto& codec : m_codecs)
  {
    // get subnode count
    auto req = this->get(0, codec.idx, 0x4);
    codec.start_node = (req.resp >> 16) & 0xFF;
    const int afg_count = req.resp & 0xff;

    for (int i = 0; i < afg_count; i++)
    {
      const int subnode = codec.start_node + i;
      // get subnode count
      auto req = this->get(subnode, codec.idx, 0x4);
      // get function group type
      auto func_group = this->get(subnode, codec.idx, 0x5);

      codec.afgs.emplace_back();
      auto& afg = codec.afgs.back();
      afg.node_id = subnode;
      afg.type    = func_group.resp;
      afg.start_node = (req.resp >> 16) & 0xff;
      afg.node_count = req.resp & 0xff;

      INFO2("|     o--[ Function Group %u ]", i);

      for (uint32_t w = 0; w < afg.node_count; w++)
      {
        afg.widgets.emplace_back();
        auto& widget = afg.widgets.back();
        widget.m_node_id = afg.start_node + w;
        // get function group type
        auto func_group = this->get(widget.nid(), codec.idx, 0x5);
        // get widget capabilities
        auto req_caps = this->get(widget.nid(), codec.idx, 0x9);
        widget.m_caps = req_caps.resp;
        // get conn list length
        auto req_cll = this->get(widget.nid(), codec.idx, 0xE);
        widget.m_conn_listen_len = req_cll.resp;
        // get conn list entries
        auto req_fcle = this->cmd(widget.nid(), codec.idx, 0xF02 << 8);
        widget.m_first_conn_list_entry = req_fcle.resp;

        INFO2("|     +-- Widget %u: %s (%#x)", w, widget.name(), widget.type());
      }
      INFO2("|");
    }
  }

  // enable interrupts

}

void IHDA::reset()
{
  // reset the device and wait
  this->write<uint32_t>(REG_GCTL, 0);
  for(int n = 0; n < 6000; n++) asm("pause");
  // start the device
  this->write<uint32_t>(REG_GCTL, 1);
  // wait for bit to reset
  while ((this->read<uint32_t>(REG_GCTL) & 0x1) == 0) asm("pause");
}

void IHDA::detect_codecs()
{
  uint8_t stat = this->read<uint8_t>(REG_STATESTS);
  for (int i = 0; i < 16; i++) {
    if (stat & (1 << i)) {
      m_codecs.emplace_back();
      m_codecs.back().idx = i;
    }
  }
}

void IHDA::setup_ring(corb_t& ring, bool corb)
{
  assert(ring.reg != 0);
  this->stop_dma(ring);
  // we only support 256 buffers (mode=2)
  // 1=0, 16=2, 256=4, resv=8
  const uint8_t sz = this->read<uint8_t> (RING_SIZE(ring));
  assert(sz & 0b0010 && "We support only 16 descriptors");
  // 0=1, 1=16, 2=256, 3=resv
  this->write<uint8_t> (RING_SIZE(ring), 0x1);
  // RIRB entries are 2x the size of CORB entries
  const size_t entry_size = (corb) ? 4 : 8;
  ring.entries = (verb_t*) memalign(128, NUM_CORB * entry_size);
  auto data = (uintptr_t) ring.entries;
  assert((data & 0x7F) == 0 && "Must be 128-byte aligned");
  this->write<uint32_t> (RING_UBASE(ring), data >> 32);
  this->write<uint32_t> (RING_LBASE(ring), data & 0xFFFFFFFF);

  if (corb)
  {
    // reset write pointer
    this->write<uint16_t> (RING_WP(ring), 0);
    // clear read pointer
    while ((this->read<uint16_t> (RING_RP(ring)) & RST_BIT) == 0) {
      this->write<uint16_t> (RING_RP(ring), RST_BIT);
      __builtin_ia32_pause();
    }
    this->write<uint16_t> (RING_RP(ring), 0x0);
  }
  else
  {
    // NOTE: reset read pointer to 1
    this->write<uint16_t> (RING_RP(ring), 1);
    // clear write pointer
    this->write<uint16_t> (RING_WP(ring), RST_BIT);
  }
}

void IHDA::start_dma(corb_t& ring)
{
  while ((this->read<uint8_t> (RING_CTL(ring)) & 0x2) != 2) {
    this->write<uint8_t> (RING_CTL(ring), 0x2); _mm_pause();
  }
}
void IHDA::stop_dma(corb_t& ring)
{
  while ((this->read<uint8_t> (RING_CTL(ring)) & 0x2) == 2) {
    this->write<uint8_t> (RING_CTL(ring), 0x0); _mm_pause();
  }
}

void IHDA::blocking_write(corb_t& ring, verb_t& verb)
{
  this->start_dma(ring);
  while (true) {
    uint16_t rp = this->read<uint16_t> (RING_RP(ring)) & 0xff;
    uint16_t wp = this->read<uint16_t> (RING_WP(ring)) & 0xff;

    __sync_synchronize();
    if (wp == rp) {
      // next free is at (WP + 4) % size
      wp = (wp + 1) % NUM_CORB;
      ring.entries[wp].whole = verb.whole;
      this->write<uint16_t> (RING_WP(ring), wp);
      return;
    }
  }
}
IHDA::response_t IHDA::blocking_read(corb_t& ring)
{
  while (true)
  {
    this->start_dma(ring);
    // RIRP responses are 64-bit (2 entries)
    uint16_t rp = this->read<uint16_t> (RING_RP(ring)) & 0xff;

    const response_t resp {
      .resp = ring.entries[rp * 2 + 0].whole,
      .rext = ring.entries[rp * 2 + 1].whole
    };

    this->stop_dma(ring);
    // advance read position
    rp = (rp + 1) % NUM_CORB;
    this->write<uint16_t> (RING_RP(ring), rp);

    this->start_dma(ring);
    return resp;
  }
}

IHDA::response_t IHDA::corb_read(verb_t& verb)
{
  // complex, and WIP
  this->blocking_write(this->m_corb, verb);
  return this->blocking_read(this->m_rirb);
}
IHDA::response_t IHDA::immediate_read(verb_t& verb)
{
  // immediate, supported by Qemu
  this->write<uint32_t> (REG_IMM_CMD, verb.whole);
  this->write<uint32_t> (REG_IMM_STAT, 0x1); // ship!
  // wait for response
  while ((this->read<uint32_t>(REG_IMM_STAT) & 0x2) == 0) _mm_pause();
  const response_t response {
      this->read<uint32_t> (REG_IMM_RESP),
      0
    };
  // response read
  this->write<uint32_t> (REG_IMM_STAT, 0x0);
  return response;
}

IHDA::response_t IHDA::cmd(uint8_t nid, uint8_t cad, uint32_t cmd)
{
  verb_t verb {
    .verb = cmd,
    .nid  = nid,
    .indirect_nid = 0,
    .cad  = cad
  };
  return immediate_read(verb);
}
IHDA::response_t IHDA::get(uint8_t nid, uint8_t cad, uint8_t param)
{
  return this->cmd(nid, cad, VERB_GET | param);
}

void IHDA::event_handler()
{
  printf("Interrupt!\n");
}

template <typename T>
inline T IHDA::read(uint32_t reg)
{
  return *(volatile T*) (this->m_hdbar + reg);
}
template <typename T>
inline void IHDA::write(uint32_t reg, T value)
{
  *(volatile T*) (this->m_hdbar + reg) = value;
}

void IHDA::set_mixer(mixer_value_t, uint32_t)
{

}
uint32_t IHDA::get_mixer(mixer_value_t)
{
  return 0;
}

const char* IHDA::widget_t::name() const noexcept
{
  switch (this->type())
  {
    case 0x0:
        return "Audio Output";
    case 0x1:
        return "Audio Input";
    case 0x4:
        return "Pin Complex";
    default:
        return "Unknown widget";
  }
}

#include <kernel/pci_manager.hpp>
__attribute__((constructor))
static void register_func()
{
  PCI_manager::register_snd(PCI::VENDOR_INTEL, 0x2668, &IHDA::new_instance);
  PCI_manager::register_snd(PCI::VENDOR_INTEL, 0x27D8, &IHDA::new_instance);
  PCI_manager::register_snd(PCI::VENDOR_AMD,   0x4383, &IHDA::new_instance);
}
