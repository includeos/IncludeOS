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
#include <hw/ioport.hpp>
#include <smp>
#include <info>
#include <cassert>
#include <malloc.h>
// loosely based on OSdev article http://wiki.osdev.org/Intel_Ethernet_i217

static const uint32_t TXDW = 1 << 0; // transmit descr written back
static const uint32_t TXQE = 1 << 1; // transmit queue empty
static const uint32_t LSC  = 1 << 2; // link status change
static const uint32_t RXDMTO= 1 << 4; // rx desc minimum tresh hit
static const uint32_t RXO  = 1 << 6; // receiver (rx) overrun
static const uint32_t RXTO = 1 << 7; // receive timer interrupt

static int deferred_event = 0;
static std::vector<e1000*> deferred_devices;

e1000::e1000(hw::PCI_Device& d) :
    Link(Link_protocol{{this, &e1000::transmit}, mac()}, bufstore_),
    m_pcidev(d), bufstore_{1024, 2048}
{
  INFO("e1000", "Intel Pro/1000 Ethernet Adapter (rev=%#x)", d.rev_id());

  // legacy IRQ from PCI
  d.enable_intx();
  uint32_t value = d.read_dword(PCI::CONFIG_INTR);
  uint8_t irq = value & 0xFF;
  assert(irq != 0xFF);

  Events::get().subscribe(irq, {this, &e1000::event_handler});
  __arch_enable_legacy_irq(irq);
  INFO2("Subscribed on IRQ %u", irq);
  this->irqs.push_back(irq);

  if (deferred_event == 0)
  {
    deferred_event = Events::get().subscribe(&e1000::do_deferred_xmit);
  }

  // shared-memory & I/O address
  this->shm_base = d.get_bar(0);
  this->use_mmio = this->shm_base > 0xFFFF;
  if (this->use_mmio == false) {
      this->io_base = d.iobase();
  }

  // initialize
  write_cmd(REG_CTRL, (1 << 26));

  // detect EEPROM
  this->detect_eeprom();

  // get MAC address
  this->retrieve_hw_addr();

  // have to clear out the multicast filter, otherwise shit breaks
	for(int i = 0; i < 128; i++)
      write_cmd(0x5200 + i*4, 0);
  for(int i = 0; i < 64; i++)
      write_cmd(0x4000 + i*4, 0);

  // enable single MAC filter
  init_filters();
  set_filter(0, this->hw_addr);

  // disables flow control
  write_cmd(0x0028, 0);
  write_cmd(0x002c, 0);
  write_cmd(0x0030, 0);
  write_cmd(0x0170, 0);

  //this->intr_cause_clear();
  this->intr_enable();

  // initialize RX
  for (int i = 0; i < NUM_RX_DESC; i++) {
    rx.desc[i].addr = (uint64_t) new_rx_packet();
    rx.desc[i].status = 0;
  }

  uint64_t rx_desc_ptr = (uint64_t) rx.desc;
  write_cmd(REG_RXDESCLO, rx_desc_ptr);
  write_cmd(REG_RXDESCHI, 0);
  write_cmd(REG_RXDESCLEN, NUM_RX_DESC * sizeof(rx_desc));
  write_cmd(REG_RXDESCHEAD, 0);
  write_cmd(REG_RXDESCTAIL, NUM_RX_DESC);
  uint32_t rx_flags =
        (1 << 1) // RX enable
      //| (1 << 3) // unicast promisc enable
      | (1 << 4) // multcast promisc enable
      | (0 << 5) // discard jumbo packets
      | (1 << 15) // broadcast accept mode
      | (0 << 16) // 2048b recv buffers
      | (1 << 26); // strip eth CRC
  write_cmd(REG_RCTRL, rx_flags);

  // initialize TX
  memset(tx.desc, 0, sizeof(tx.desc));
  for (int i = 0; i < NUM_TX_DESC; i++) {
    tx.desc[i].status = 0x1; // done
  }

  uint64_t tx_desc_ptr = (uint64_t) tx.desc;
  write_cmd(REG_TXDESCLO, tx_desc_ptr);
  write_cmd(REG_TXDESCHI, 0);
  write_cmd(REG_TXDESCLEN, NUM_TX_DESC * sizeof(tx_desc));
  write_cmd(REG_TXDESCHEAD, 0);
  write_cmd(REG_TXDESCTAIL, NUM_TX_DESC);

  // enable, PSP, 0xF coll tresh, 0x3F coll distance
  //uint32_t tctrl = read_cmd(REG_TCTRL);
  //write_cmd(REG_TCTRL, tctrl | (1 << 1) | (1 << 3));
  write_cmd(REG_TCTRL, (1 << 1) | (1 << 3));
  //write_cmd(REG_TIPG, 0x702008); // p.202

  // set link up command
  this->link_up();

  // GO!
  uint32_t flags = read_cmd(REG_RCTRL);
  write_cmd(REG_RCTRL, flags | RCTL_EN);
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
  if (LIKELY(this->use_mmio))
      *(volatile uint32_t*) (this->shm_base + cmd) = val;
  hw::outl(this->io_base, cmd);
  hw::outl(this->io_base + 4, val);
}

void e1000::retrieve_hw_addr()
{
  if (this->use_eeprom)
  {
    uint16_t* mac = &hw_addr.minor;
    mac[0] = read_eeprom(0) & 0xFFFF;
    mac[1] = read_eeprom(1) & 0xFFFF;
    mac[2] = read_eeprom(2) & 0xFFFF;
  }
  else
  {
    auto* mac_src = (const char*) (this->shm_base + 0x5400);
    memcpy(&this->hw_addr, mac_src, sizeof(hw_addr));
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
  write_cmd(REG_EEPROM, 0x1);
  for (int i = 0; i < 1000; i++)
  {
    uint32_t val = read_cmd(REG_EEPROM);
    if (val & 0x10) {
      this->use_eeprom = true;
      return;
    }
  }
}
uint32_t e1000::read_eeprom(uint8_t addr)
{
	uint32_t tmp = 0;
  if (this->use_eeprom)
  {
    write_cmd(REG_EEPROM, 1 | ((uint32_t)(addr) << 8));
  	while (!((tmp = read_cmd(REG_EEPROM)) & (1 << 4)) );
  }
  return (tmp >> 16) & 0xFFFF;
}

void e1000::link_up()
{
  uint32_t flags = read_cmd(REG_CTRL);
  write_cmd(REG_CTRL, flags | ECTRL_SLU);

  uint32_t status = read_cmd(REG_STATUS);
  int success = (status & (1 << 1)) != 0;
  if (success == 0)
      INFO("e1000", "Link NOT up");
  else {
    const char* spd = ((status >> 6) & 3) ? "1000Mb/s" : "100Mb/s";
    const char* duplex = (status & 1) ? "Full Duplex" : "Half Duplex";
    INFO("e1000", "Link up at %s %s", spd, duplex);
  }
}

void e1000::intr_enable()
{
  write_cmd(REG_IMASK, TXDW | TXQE | LSC | RXDMTO | RXO | RXTO);
}
void e1000::intr_disable()
{
  write_cmd(REG_IMC, TXDW | TXQE | LSC | RXDMTO | RXO | RXTO);
}
void e1000::intr_cause_clear()
{
  write_cmd(REG_ICRR, 0x5FD2F7);
}

net::Packet_ptr
e1000::recv_packet(uint8_t* data, uint16_t size)
{
  auto* ptr = (net::Packet*) (data - DRIVER_OFFSET - sizeof(net::Packet));
  new (ptr) net::Packet(
        DRIVER_OFFSET,
        size,
        DRIVER_OFFSET + packet_len(),
        &bufstore());
  return net::Packet_ptr(ptr);
}
net::Packet_ptr
e1000::create_packet(int link_offset)
{
  auto buffer = bufstore().get_buffer();
  auto* ptr = (net::Packet*) buffer.addr;
  new (ptr) net::Packet(
        DRIVER_OFFSET + link_offset,
        0,
        DRIVER_OFFSET + packet_len(),
        buffer.bufstore);
  return net::Packet_ptr(ptr);
}
uintptr_t e1000::new_rx_packet()
{
  auto* pkt = bufstore().get_buffer().addr;
  return (uintptr_t) &pkt[sizeof(net::Packet) + DRIVER_OFFSET];
}

void e1000::event_handler()
{
  uint32_t status = read_cmd(0xC0);
  // see: e1000_regs.h
  printf("e1000: event %x received\n", status);

  // empty transmit queue
  if (status & TXQE)
  {
    //printf("tx queue empty!\n");
    if (sendq) {
      transmit(std::move(sendq));
    }
    if (can_transmit()) {
      transmit_queue_available_event(NUM_TX_DESC);
    }
  }
  // link status change
  if (status & LSC)
  {
    this->link_up();
  }
  if (status & RXDMTO)
  {
    printf("e1000: rx descriptor minimum treshold hit!\n");
  }
  if (status & RXO)
  {
    printf("e1000: rx overrun!\n");
  }
  // rx timer interrupt
  if (status & RXTO)
  {
    recv_handler();
  }

  // ready to handle more events
  //this->intr_cause_clear();
}

void e1000::recv_handler()
{
  uint16_t old_idx = 0xffff;

  while (rx.desc[rx.current].status & 1)
  {
    auto& tk = rx.desc[rx.current];
    auto* buf = (uint8_t*) tk.addr;

    printf("e1000: recv %u bytes -> %p\n", tk.length, buf);
    auto pkt = recv_packet(buf, tk.length);
    Link_layer::receive(std::move(pkt));

    // give new buffer
    tk.addr = (uint64_t) this->new_rx_packet();
    tk.status = 0;
    // go to next index
    old_idx = rx.current;
    rx.current = (rx.current + 1) % NUM_RX_DESC;
  }
  if (old_idx != 0xffff)
    write_cmd(REG_RXDESCTAIL, old_idx);
}

void e1000::transmit(net::Packet_ptr pckt)
{
  if (sendq == nullptr)
      sendq = std::move(pckt);
  else
      sendq->chain(std::move(pckt));
  // send as much as possible from sendq
  while (sendq != nullptr && can_transmit())
  {
    auto next = sendq->detach_tail();
    // transmit released buffer
    auto* packet = sendq.release();
    transmit_data(packet->buf() + DRIVER_OFFSET, packet->size());
    // next is the new sendq
    sendq = std::move(next);
  }
}
bool e1000::can_transmit()
{
  return (tx.desc[tx.current].status & 0xFF) == 0x1;
}
void e1000::transmit_data(uint8_t* data, uint16_t length)
{
  auto& tk = tx.desc[tx.current];
  assert(tk.status == 0x1 && "Descriptor must be done");

  if (tk.addr != 0x0) {
    auto* packet = (net::Packet*) (tk.addr - DRIVER_OFFSET - sizeof(net::Packet));
    delete packet; // call deleter on Packet to release it
  }
  printf("e1000: xmit %p -> %u bytes\n", data, length);
  tk.addr   = (uint64_t) data;
  tk.length = length;
  tk.cso    = 0;
  tk.cmd    = (1 << 3) | 0x3;
  tk.status = 0;
  tk.css    = 0;
  tk.special = 0;

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
  write_cmd(REG_TXDESCTAIL, tx.current);
  tx.deferred = false;
}
void e1000::do_deferred_xmit()
{
  for (auto& dev : deferred_devices)
      dev->xmit_kick();
  deferred_devices.clear();
}

void e1000::flush()
{
  this->transmit(std::move(sendq));
}
void e1000::poll()
{
  this->recv_handler();
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
  PCI_manager::register_nic(PCI::VENDOR_INTEL, 0x10EA, &e1000::new_instance);
  // I217
  PCI_manager::register_nic(PCI::VENDOR_INTEL, 0x153A, &e1000::new_instance);
  PCI_manager::register_nic(PCI::VENDOR_INTEL, 0x1539, &e1000::new_instance);
  // I219
  PCI_manager::register_nic(PCI::VENDOR_INTEL, 0x15B8, &e1000::new_instance);
}
