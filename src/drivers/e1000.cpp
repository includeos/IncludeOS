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

#include "e1000.hpp"
#include "e1000_defs.hpp"
#include <kernel/events.hpp>
#include <kernel/timers.hpp>
#include <os.hpp>
#include <hw/ioport.hpp>
#include <info>
#include <cassert>
//#define E1000_ENABLE_STATS
//#define E1000_FAKE_EVENT_HANDLER
//#define E1000E_FAKE_EVENT_HANDLER

//#define VERBOSE_E1000
#ifdef VERBOSE_E1000
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif

static const uint32_t TXDW = 1 << 0; // transmit descr written back
static const uint32_t TXQE = 1 << 1; // transmit queue empty
static const uint32_t LSC  = 1 << 2; // link status change
static const uint32_t RXDMTO= 1 << 4; // rx desc minimum tresh hit
static const uint32_t RXO  = 1 << 6; // receiver (rx) overrun
static const uint32_t RXTO = 1 << 7; // receive timer interrupt
#define LEGACY_INTR_MASK() (TXDW | TXQE | LSC | RXDMTO | RXO | RXTO)
#define MSIX_INTR_MASK() (LSC | RXO | (1 << 20) | (1 << 22) | (1 << 24))

static int deferred_event = 0;
static std::vector<e1000*> deferred_devices;

static inline uint16_t report_size_for_mtu(uint16_t mtu)
{
  uint16_t diff = mtu % 1024;
  mtu = (mtu / 1024) * 1024;
  if (diff) mtu += 1024;
  return mtu;
}
static inline uint16_t buffer_size_for_mtu(const uint16_t mtu)
{
  const uint16_t header = sizeof(net::Packet) + e1000::DRIVER_OFFSET;
  const uint16_t total = header + sizeof(net::ethernet::VLAN_header) + mtu;
  if (total <= 2048) return 2048;
  assert(mtu <= 9000 && "Buffers larger than 9000 are not supported");
  return total;
}

#define NUM_PACKET_BUFFERS (NUM_TX_DESC + NUM_RX_DESC + NUM_TX_QUEUE + 8)

e1000::e1000(hw::PCI_Device& d, uint16_t mtu) :
    Link(Link_protocol{{this, &e1000::transmit}, mac()}),
    m_pcidev(d), m_mtu(mtu), bufstore_{NUM_PACKET_BUFFERS, buffer_size_for_mtu(mtu)}
{
  static_assert((NUM_RX_DESC * sizeof(rx_desc)) % 128 == 0, "Ring length must be 128-byte aligned");
  static_assert((NUM_TX_DESC * sizeof(tx_desc)) % 128 == 0, "Ring length must be 128-byte aligned");

  INFO("e1000", "Intel Pro/1000 Ethernet Adapter (rev=%#x)", d.rev_id());
  // find and store capabilities
  d.parse_capabilities();
  // find BARs etc.
  d.probe_resources();

  this->use_msix = false;
  const uint16_t variant = d.product_id();

  // exception for I217
  if (variant == 0x153A || variant == 0x1539)
  {
    this->use_msix = false;
  }
  else if (d.msix_cap())
  {
    d.init_msix();
    if (d.has_msix())
    {
      this->use_msix = true;
    }
  }
  // legacy INTx IRQ from PCI
  if (this->use_msix == false)
  {
    // 21 = lots of times (???)
    // 16 = USB
    // 18 = e1000 I217
    uint32_t value = d.read32(PCI::CONFIG_INTR);
    uint8_t real_irq = value & 0xFF;
    assert(real_irq != 0xFF);

    if (variant == 0x153A || variant == 0x1539)
    {
      // I217 bare metal IRQ on AX370
      static const uint8_t I217_IRQ = 18;
      __arch_enable_legacy_irq(I217_IRQ);
      Events::get().subscribe(I217_IRQ,
      [this, real_irq] () {
        printf("Received interrupt %u (PCI IRQ %u)\n", I217_IRQ, real_irq);
        this->event_handler();
      });
    }
    /*
    for (unsigned i = 8; i < 24+32; i++)
    {
      if (i == 21) continue;
      __arch_enable_legacy_irq(i);
      Events::get().subscribe(i,
      [this, i] () {
        printf("Received interrupt %u\n", i);
        this->event_handler();
      });
    }*/

    // real thing
    __arch_enable_legacy_irq(real_irq);
    Events::get().subscribe(real_irq, {this, &e1000::event_handler});
    this->irqs.push_back(real_irq);
  }

  if (deferred_event == 0)
  {
    deferred_event = Events::get().subscribe(&e1000::do_deferred_xmit);
  }

  // shared-memory & I/O address
  this->shm_base = d.get_bar(0).start;
  this->use_mmio = this->shm_base > 0xFFFF;
  if (this->use_mmio == false) {
      this->io_base = d.iobase();
  }

  // detect EEPROM
  this->detect_eeprom();

  // get MAC address
  this->retrieve_hw_addr();

  // SW reset device
  write_cmd(REG_CTRL, (1 << 26));
  while (read_cmd(REG_CTRL) & (1 << 26)) {
    asm("pause");
  }

  // have to clear out the multicast filter, otherwise shit breaks
	for (int i = 0; i < 128; i++)
      write_cmd(0x5200 + i*4, 0);
  for (int i = 0; i < 64; i++)
      write_cmd(0x4000 + i*4, 0);

  // enable single MAC filter
  //init_filters();
  //set_filter(0, this->hw_addr);

  // disables flow control
  write_cmd(0x0028, 0);
  write_cmd(0x002c, 0);
  write_cmd(0x0030, 0);
  write_cmd(0x0170, 0);
  write_cmd(REG_CTRL, read_cmd(REG_CTRL) & ~(1 << 30));

  this->intr_cause_clear();
  this->intr_enable();

  // initialize RX
  for (int i = 0; i < NUM_RX_DESC; i++) {
    rx.desc[i].addr = (uint64_t) new_rx_packet();
    rx.desc[i].status = 0;
  }

  write_cmd(REG_RXDESCLO, (uint64_t) rx.desc);
  write_cmd(REG_RXDESCHI, 0);
  write_cmd(REG_RXDESCLEN, NUM_RX_DESC * sizeof(rx_desc));
  write_cmd(REG_RXDESCHEAD, 0);
  write_cmd(REG_RXDESCTAIL, NUM_RX_DESC-1);
  uint32_t rx_flags = 0
      //| RCTL_UPE // unicast promisc enable
      | RCTL_MPE // multicast promisc enable
      | RCTL_BAM // broadcast accept mode
      | (report_size_for_mtu(MTU()) << 27) // recv buffers
      | RCTL_SECRC; // strip eth CRC
  // LPE if MTU > 1500
  if (MTU() > 1500) rx_flags |= RCTL_LPE;
  write_cmd(REG_RCTRL, rx_flags);

  // initialize TX
  memset(tx.desc, 0, sizeof(tx.desc));
  for (int i = 0; i < NUM_TX_DESC; i++) {
    tx.desc[i].status = 0x1; // done
  }

  write_cmd(REG_TXDESCLO, (uint64_t) tx.desc);
  write_cmd(REG_TXDESCHI, 0);
  write_cmd(REG_TXDESCLEN, NUM_TX_DESC * sizeof(tx_desc));
  write_cmd(REG_TXDESCHEAD, 0);
  write_cmd(REG_TXDESCTAIL, NUM_TX_DESC-1);
  //write_cmd(REG_TIPG, 0x702008); // p.202
  //write_cmd(REG_TIPG, (10 | (10 << 10) | (10 << 20)));
  // enable, PSP, 0xF coll tresh, 0x3F coll distance
  write_cmd(REG_TCTRL, read_cmd(REG_TCTRL) | (1 << 1) | (1 << 3));

  // e1000e configuration
  if (this->use_msix)
  {
    // interrupt auto-clear for vec0 - vec4
    write_cmd(REG_EIAC, 0xFFFFFFFF);
    write_cmd(REG_IAM, 0x0);

    // configure IVAR and MSI-X table entries
    this->config_msix();

    // CTRL_EXT = MSI-X PBA + **normal** IAME
    write_cmd(REG_CTRL_EXT, (1 << 31) | (1 << 27));
  }
  else
  {
    write_cmd(REG_ITR, 100);
    //write_cmd(REG_IAM, 0x0);
    // CTRL_EXT = IAME
    //write_cmd(REG_CTRL_EXT, (1 << 27));
  }

  // remove master disable bit
  write_cmd(REG_CTRL, read_cmd(REG_CTRL) & ~(1 << 2));
  // assert driver loaded (DRV_LOAD)
  write_cmd(REG_CTRL_EXT, read_cmd(REG_CTRL_EXT) | (1 << 28));

  // set link up command
  this->link_up();

  // GO!
  write_cmd(REG_RCTRL, read_cmd(REG_RCTRL) | RCTL_EN);

#ifdef E1000_ENABLE_STATS
  Timers::periodic(std::chrono::seconds(2),
    [this] (int) {
      uint32_t stat = this->read_cmd(REG_STATUS);
      //printf("Full Duplex: %u\n", stat & (1 << 0));
      const char* spd = ((stat >> 6) & 3) ? "1000Mb/s" : "100Mb/s";
      printf("Link Up: %u (%s)\n", stat & (1 << 1), spd);
      //printf("PHY powered off: %u\n", stat & (1 << 5));
      printf("Transmission paused: %u\n", stat & (1 << 4));
      printf("Read comp blocked: %u\n", stat & (1 << 8));
      //printf("LAN init done: %u\n", stat & (1 << 9));
      //printf("Master enable status: %u\n", stat & (1 << 19));
      printf("\n");
      uint64_t val;
      val = this->read_cmd(0x40C0);
      val |= (uint64_t) this->read_cmd(0x40C4) << 32;
      printf("Octets received: %lu\n", val);
      val = this->read_cmd(0x40C8);
      val |= (uint64_t) this->read_cmd(0x40CC) << 32;
      printf("Octets transmitted: %lu\n", val);
      printf("Packets RX total: %u\n", this->read_cmd(0x40D0));
      printf("Packets TX total: %u\n", this->read_cmd(0x40D4));
      printf("Intr enabled: %x\n", this->read_cmd(REG_IMS));
      printf("Intr caused:  %x\n", this->read_cmd(REG_ICRR));
      printf("\n");
    });
#endif
#ifdef E1000_FAKE_EVENT_HANDLER
  Timers::periodic(std::chrono::milliseconds(1),
    [this] (int) {
      this->event_handler();
    });
#endif
#ifdef E1000E_FAKE_EVENT_HANDLER
  Timers::periodic(std::chrono::milliseconds(1),
    [this] (int) {
      this->receive_handler();
      this->transmit_handler();
      this->event_handler();
    });
#endif
}

void e1000::config_msix()
{
  #define IVAR_INT_ALLOC_VALID 0x8 // 10.2.4.9 p.328
  uint32_t ivar = 0;
  // rx queue 0 2:0
  uint8_t vec0 = Events::get().subscribe({this, &e1000::receive_handler});
  int m0 = m_pcidev.setup_msix_vector(SMP::cpu_id(), IRQ_BASE + vec0);
  ivar |= (IVAR_INT_ALLOC_VALID | m0);

  // tx queue 0 10:8
  uint8_t vec1 = Events::get().subscribe({this, &e1000::transmit_handler});
  int m1 = m_pcidev.setup_msix_vector(SMP::cpu_id(), IRQ_BASE + vec1);
  ivar |= (IVAR_INT_ALLOC_VALID | m1) << 8;

  // other causes 18:16
  uint8_t vec2 = Events::get().subscribe({this, &e1000::event_handler});
  int m2 = m_pcidev.setup_msix_vector(SMP::cpu_id(), IRQ_BASE + vec2);
  ivar |= (IVAR_INT_ALLOC_VALID | m2) << 16;

  // enable TX interrupts on every writeback regardless of RS bit
  ivar |= 1 << 31;
  write_cmd(REG_IVAR, ivar);
}

void e1000::wait_millis(int millis)
{
  bool done_waiting = false;
  Timers::oneshot(std::chrono::milliseconds(millis),
    [&done_waiting] (int) {
      done_waiting = true;
    });
  Events::get().process_events();
  while (done_waiting == false) {
    os::halt();
    Events::get().process_events();
  }
}

uint32_t e1000::read_cmd(uint16_t cmd)
{
  if (LIKELY(this->use_mmio))
      return *(volatile uint32_t*) (this->shm_base + cmd);
  hw::outl(this->io_base, cmd);
  return hw::inl(this->io_base + 4);
}
void e1000::write_cmd(uint16_t cmd, uint32_t val)
{
  if (LIKELY(this->use_mmio)) {
      *(volatile uint32_t*) (this->shm_base + cmd) = val;
      return;
  }
  hw::outl(this->io_base, cmd);
  hw::outl(this->io_base + 4, val);
}

void e1000::retrieve_hw_addr()
{
  uint16_t* mac = &hw_addr.minor;
  const uint32_t ral = read_cmd(0x5400);
  if (ral)
  {
    const uint32_t rah = read_cmd(0x5404);
    mac[0] = ral;
    mac[1] = ral >> 16;
    mac[2] = rah;
  }
  else if (this->use_eeprom)
  {
    mac[0] = read_eeprom(0) & 0xFFFF;
    mac[1] = read_eeprom(1) & 0xFFFF;
    mac[2] = read_eeprom(2) & 0xFFFF;
  }
  else
  {
    assert(0 && "e1000e: Don't know how to read MAC address");
  }
  INFO2("MAC address: %s", hw_addr.to_string().c_str());
}

void e1000::init_filters()
{
  uint64_t filters[16];
  memset(filters, 0, sizeof(filters));
  auto* scan = (uint32_t*) &filters[0];
  for (int i = 0; i < 32; i++)
  {
    write_cmd(0x5400 + i*4, scan[i]);
  }
}
void e1000::set_filter(int idx, MAC::Addr addr)
{
  assert(idx >= 0 && idx < 16);
  uint64_t filter = construct_filter(addr);
  for (int i = 0; i < 2; i++)
    write_cmd(0x5400 + idx*8 + i*4, ((uint32_t*) &filter)[i]);
}

uint64_t e1000::construct_filter(MAC::Addr addr)
{
  uint64_t filter = 0;
  memcpy(((char*) &filter), &addr, 6);
  return filter | (1ull << 63);
}

void e1000::detect_eeprom()
{
  write_cmd(REG_EEC, 0x1);
  const uint32_t eec = read_cmd(REG_EEC);
  const bool NVM_present = (eec & (1 << 8)) == 1;
  this->use_eeprom = NVM_present && (eec & (1 << 23)) == 0;
  INFO2("NVM present: %d  type: %s", NVM_present, (use_eeprom) ? "EEPROM" : "Flash");
  this->use_eeprom = true;
}
uint32_t e1000::read_eeprom(uint8_t addr)
{
  // start read
  write_cmd(REG_EEPROM, 1 | ((uint32_t)(addr) << 8));
  // wait for word-read complete
	while ((read_cmd(REG_EEPROM) & (1 << 1)) == 0) {
    asm("pause");
  }
  return (read_cmd(REG_EEPROM) >> 16) & 0xFFFF;
}

void e1000::link_up()
{
  // set link up CTRL.SLU (not present on modern devices)
  write_cmd(REG_CTRL, read_cmd(REG_CTRL) | (1 << 6));
  //wait_millis(1);

  uint32_t status = read_cmd(REG_STATUS);
  this->link_state_up = (status & (1 << 1)) != 0;
  if (this->link_state_up == false)
      INFO("e1000", "Link NOT up");
  else {
    const char* spd = ((status >> 6) & 3) ? "1000Mb/s" : "100Mb/s";
    const char* duplex = (status & 1) ? "Full Duplex" : "Half Duplex";
    INFO("e1000", "Link up at %s %s", spd, duplex);
  }
}

void e1000::intr_enable()
{
  if (this->use_msix)
    write_cmd(REG_IMS, MSIX_INTR_MASK());
  else
    write_cmd(REG_IMS, LEGACY_INTR_MASK());
}
void e1000::intr_disable()
{
  if (this->use_msix)
    write_cmd(REG_IMC, MSIX_INTR_MASK());
  else
    write_cmd(REG_IMC, LEGACY_INTR_MASK());
}
void e1000::intr_cause_clear()
{
  write_cmd(REG_ICRR, 0xFFFFFFFF);
}

net::Packet_ptr
e1000::recv_packet(uint8_t* data, uint16_t size)
{
  auto* ptr = (net::Packet*) (data - DRIVER_OFFSET - sizeof(net::Packet));
  new (ptr) net::Packet(
        DRIVER_OFFSET,
        size,
        DRIVER_OFFSET + size,
        &bufstore());
  return net::Packet_ptr(ptr);
}
net::Packet_ptr
e1000::create_packet(int link_offset)
{
  auto* ptr = (net::Packet*) bufstore().get_buffer();
  new (ptr) net::Packet(
        DRIVER_OFFSET + link_offset,
        0,
        DRIVER_OFFSET + frame_offset_link() + MTU(),
        &bufstore());
  return net::Packet_ptr(ptr);
}
uintptr_t e1000::new_rx_packet()
{
  auto* pkt = bufstore().get_buffer();
  return (uintptr_t) &pkt[sizeof(net::Packet) + DRIVER_OFFSET];
}

void e1000::event_handler()
{
  const uint32_t status = read_cmd(REG_ICRR);
  if (status == 0) {
    PRINT("[e1000] spurious event handler\n");
    return;
  }
  // see: e1000_regs.h
  PRINT("[e1000] event %x received\n", status);

  // empty tx queue or tx desc written back
  if (status & TXQE || status & TXDW)
  {
    // free old & send more!
    this->transmit_handler();
  }
  // link status change
  if (status & LSC)
  {
    PRINT("[e1000] link state changed\n");
    this->link_up();
  }
  if (status & RXDMTO)
  {
    PRINT("[e1000] rx descriptor minimum treshold hit!\n");
    this->receive_handler();
  }
  if (status & RXO)
  {
    fprintf(stderr, "[e1000] rx overrun!\n");
  }
  // rx timer interrupt
  if (status & RXTO)
  {
    this->receive_handler();
  }

  // ready to handle more events
  this->intr_cause_clear();
}

void e1000::receive_handler()
{
  uint16_t old_idx = 0;
  uint32_t received = 0;
  std::array<net::Packet_ptr, NUM_RX_DESC> recv_array;

  while (true)
  {
    auto& tk = rx.desc[rx.current];
    if ((tk.status & 1) == 0) break;

    // must be complete packet
    if ((tk.status & 2) == 0) {
      PRINT("[e1000] dropping incomplete buffer %u bytes\n", tk.length);
      continue;
    }

    auto* buf = (uint8_t*) tk.addr;
    assert(buf != nullptr);
    PRINT("[e1000] recv %p -> %u bytes\n", buf, tk.length);

    recv_array[received] = recv_packet(buf, tk.length);
    received++;

    // give new buffer
    tk.addr = (uint64_t) this->new_rx_packet();
    tk.status = 0;
    // go to next index
    old_idx = rx.current;
    rx.current = (rx.current + 1) % NUM_RX_DESC;
  }

  if (received > 0)
  {
    // acknowledge all rx packets
    write_cmd(REG_RXDESCTAIL, old_idx);
    // process rx packets
    for (uint32_t i = 0; i < received; i++) {
      Link_layer::receive(std::move(recv_array[i]));
    }
  }
}

void e1000::transmit_handler()
{
  // try to free transmitted buffers
  do_release_transmitted();

  // try to send more packets
  if (sendq) {
    transmit(nullptr);
  }
  if (can_transmit() || sendq_size < NUM_TX_QUEUE) {
    const uint32_t free_tokens = free_transmit_descr() + (NUM_TX_QUEUE - sendq_size);
    //printf("free_tokens: %u\n", free_tokens);
    transmit_queue_available_event(free_tokens);
  }
}
bool e1000::can_transmit() const noexcept
{
  return this->link_state_up && tx.sent.size() < NUM_TX_DESC;
}
uint16_t e1000::free_transmit_descr() const noexcept
{
  return NUM_TX_DESC - tx.sent.size();
}

void e1000::transmit(net::Packet_ptr pckt)
{
  if (pckt != nullptr) {
      sendq_size += pckt->chain_length();
      if (sendq == nullptr)
        sendq = std::move(pckt);
      else
        sendq->chain(std::move(pckt));
  }

  // send as much as possible from sendq
  while (sendq != nullptr && can_transmit())
  {
    auto next = sendq->detach_tail();
    // transmit released buffer
    auto* packet = sendq.release();
    transmit_data(packet->buf() + DRIVER_OFFSET, packet->size());
    // add to sent packets
    tx.sent.push_back(packet);
    // decrement send queue size
    assert(sendq_size > 0);
    sendq_size--;
    // force kick if tx queue is exactly quarter full
    if (this->free_transmit_descr() == NUM_TX_DESC / 4) {
      this->xmit_kick();
    }
    // next is the new sendq
    sendq = std::move(next);
  }
}
void e1000::do_release_transmitted()
{
  //const uint16_t tx_head = read_cmd(REG_TXDESCTAIL);
  //uint32_t total = (NUM_TX_DESC - tx.current + tx_head) % NUM_TX_DESC;
  while (not tx.sent.empty())
  {
    auto& tk = tx.desc[tx.sent_id];
    if ((tk.status & 0x1) == 0) break;

    auto* packet = tx.sent.front();
    delete packet; // call deleter on Packet to release it
    tx.sent.pop_front();
    tx.sent_id = (tx.sent_id + 1) % NUM_TX_DESC;
  }
  PRINT("[e1000] transmitting %lu - bufferstore %lu\n",
        tx.sent.size(), bufstore().available());
}
void e1000::transmit_data(uint8_t* data, uint16_t length)
{
  auto& tk = tx.desc[tx.current];
  assert(tk.status == 0x1 && "Descriptor must be done");

  PRINT("[e1000] xmit %p -> %u bytes\n", data, length);
  tk.addr   = (uint64_t) data;
  tk.length = length;
  tk.cso    = 0;
  tk.cmd    = 0x3 | (1 << 3);
  tk.status = 0;
  tk.css    = 0;
  tk.vlan_tag = 0;
  // next tx position
  tx.current = (tx.current + 1) % NUM_TX_DESC;

  if (tx.deferred == false)
  {
    tx.deferred = true;
    deferred_devices.push_back(this);
    Events::get().trigger_event(deferred_event);
  }
}
void e1000::xmit_kick()
{
  if (tx.deferred) {
    tx.deferred = false;
    write_cmd(REG_TXDESCTAIL, tx.current);
  }
}
void e1000::do_deferred_xmit()
{
  for (auto& dev : deferred_devices)
      dev->xmit_kick();
  deferred_devices.clear();
}

void e1000::flush()
{
  this->transmit(std::move(nullptr));
}
void e1000::poll()
{
  this->receive_handler();
}
void e1000::deactivate()
{
  uint32_t flags = read_cmd(REG_RCTRL);
  write_cmd(REG_RCTRL, flags & ~RCTL_EN);
}
void e1000::move_to_this_cpu()
{
  // TODO: implement me
}

#include <kernel/pci_manager.hpp>
__attribute__((constructor))
static void register_func()
{
  // e1000
  PCI_manager::register_nic(PCI::VENDOR_INTEL, 0x100E, &e1000::new_instance);
  PCI_manager::register_nic(PCI::VENDOR_INTEL, 0x100F, &e1000::new_instance);
  PCI_manager::register_nic(PCI::VENDOR_INTEL, 0x109A, &e1000::new_instance);
  // e1000e 82574L
  PCI_manager::register_nic(PCI::VENDOR_INTEL, 0x10D3, &e1000::new_instance);
  PCI_manager::register_nic(PCI::VENDOR_INTEL, 0x10EA, &e1000::new_instance);
  // I217
  PCI_manager::register_nic(PCI::VENDOR_INTEL, 0x153A, &e1000::new_instance);
  PCI_manager::register_nic(PCI::VENDOR_INTEL, 0x1539, &e1000::new_instance);
  // I219
  PCI_manager::register_nic(PCI::VENDOR_INTEL, 0x15B8, &e1000::new_instance);
}
