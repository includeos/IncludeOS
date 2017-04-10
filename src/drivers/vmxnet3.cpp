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

// loosely based on iPXE driver as well as from Linux driver by VMware
#include "vmxnet3.hpp"
#include "vmxnet3_queues.hpp"

#include <kernel/irq_manager.hpp>
#include <smp>
#include <info>
#include <cassert>
#include <malloc.h>
static std::vector<vmxnet3*> deferred_devs;

#define VMXNET3_REV1_MAGIC 0xbabefee1
#define VMXNET3_MAX_BUFFER_LEN 0x4000
#define VMXNET3_DMA_ALIGN  512

#define VMXNET3_NUM_TX_COMP  vmxnet3::NUM_TX_DESC
#define VMXNET3_NUM_RX_COMP  vmxnet3::NUM_RX_DESC
static const int VMXNET3_TX_FILL = vmxnet3::NUM_TX_DESC-1;
static const int VMXNET3_RX_FILL = vmxnet3::NUM_RX_DESC;

/**
 * DMA areas
 *
 * These are arranged in order of decreasing alignment, to allow for a
 * single allocation
 */
struct vmxnet3_dma {
  /** TX ring */
  struct vmxnet3_tx_desc tx_desc[vmxnet3::NUM_TX_DESC];
  struct vmxnet3_tx_comp tx_comp[VMXNET3_NUM_TX_COMP];
  /** RX ring */
  struct vmxnet3_rx_desc rx0_desc[vmxnet3::NUM_RX_DESC];
  struct vmxnet3_rx_desc rx1_desc[vmxnet3::NUM_RX_DESC];
  struct vmxnet3_rx_comp rx_comp[VMXNET3_NUM_RX_COMP];
  /** Queue descriptors */
  struct vmxnet3_queues queues;
  /** Shared area */
  struct vmxnet3_shared shared;

} __attribute__ ((aligned(VMXNET3_DMA_ALIGN)));

#define PRODUCT_ID    0x7b0
#define REVISION_ID   0x1

#define PCI_BAR_PT     0
#define PCI_BAR_VD     1
#define PCI_BAR_MSIX   2

#define VMXNET3_VD_CMD    0x20
#define VMXNET3_VD_MAC_LO 0x28
#define VMXNET3_VD_MAC_HI 0x30
#define VMXNET3_VD_ECR    0x40

/** Commands */
enum vmxnet3_command {
  VMXNET3_CMD_FIRST_SET = 0xcafe0000,
  VMXNET3_CMD_ACTIVATE_DEV = VMXNET3_CMD_FIRST_SET,
  VMXNET3_CMD_QUIESCE_DEV,
  VMXNET3_CMD_RESET_DEV,
  VMXNET3_CMD_UPDATE_RX_MODE,
  VMXNET3_CMD_UPDATE_MAC_FILTERS,
  VMXNET3_CMD_UPDATE_VLAN_FILTERS,
  VMXNET3_CMD_UPDATE_RSSIDT,
  VMXNET3_CMD_UPDATE_IML,
  VMXNET3_CMD_UPDATE_PMCFG,
  VMXNET3_CMD_UPDATE_FEATURE,
  VMXNET3_CMD_LOAD_PLUGIN,

  VMXNET3_CMD_FIRST_GET = 0xf00d0000,
  VMXNET3_CMD_GET_QUEUE_STATUS = VMXNET3_CMD_FIRST_GET,
  VMXNET3_CMD_GET_STATS,
  VMXNET3_CMD_GET_LINK,
  VMXNET3_CMD_GET_PERM_MAC_LO,
  VMXNET3_CMD_GET_PERM_MAC_HI,
  VMXNET3_CMD_GET_DID_LO,
  VMXNET3_CMD_GET_DID_HI,
  VMXNET3_CMD_GET_DEV_EXTRA_INFO,
  VMXNET3_CMD_GET_CONF_INTR
};

inline uint32_t mmio_read32(uintptr_t location)
{
  return *(uint32_t volatile*) location;
}
inline void mmio_write32(uintptr_t location, uint32_t value)
{
  *(uint32_t volatile*) location = value;
}

vmxnet3::vmxnet3(hw::PCI_Device& d) :
    Link(Link_protocol{{this, &vmxnet3::transmit}, mac()},
         2048, 2048 /* half-page buffer size */),
    pcidev(d)
{
  INFO("vmxnet3", "Driver initializing (rev=%#x)", d.rev_id());
  assert(d.rev_id() == REVISION_ID);

  if (d.has_msix())
  {
    uint8_t msix_vectors = d.get_msix_vectors();
    INFO2("[x] Device has %u MSI-X vectors", msix_vectors);
    assert(msix_vectors >= 3);
    if (msix_vectors > 3) msix_vectors = 3;

    for (int i = 0; i < msix_vectors; i++)
    {
      auto irq = IRQ_manager::get().get_free_irq();
      d.setup_msix_vector(SMP::cpu_id(), IRQ_BASE + irq);
      this->irqs.push_back(irq);
      // dummpy subscription to make free_irq change
      IRQ_manager::get().subscribe(irq, nullptr);
    }

    IRQ_manager::get().subscribe(irqs[0], {this, &vmxnet3::msix_evt_handler});
    IRQ_manager::get().subscribe(irqs[1], {this, &vmxnet3::msix_xmit_handler});
    IRQ_manager::get().subscribe(irqs[2], {this, &vmxnet3::msix_recv_handler});
  }
  else {
    assert(0 && "This driver does not support legacy IRQs");
  }

  // dma areas
  this->iobase = d.get_bar(PCI_BAR_VD);
  assert(this->iobase);
  this->ptbase = d.get_bar(PCI_BAR_PT);
  assert(this->ptbase);

  // verify and select version
  bool ok = check_version();
  assert(ok);

  // reset device
  ok = reset();
  assert(ok);

  // get mac address
  retrieve_hwaddr();

  // check link status
  auto link_spd = check_link();
  if (link_spd) {
    INFO2("Link up at %u Mbps", link_spd);
  }
  else {
    INFO2("LINK DOWN! :(");
    return;
  }

  // set MAC
  set_hwaddr(this->hw_addr);

  // initialize DMA areas
  this->dma = (vmxnet3_dma*) memalign(VMXNET3_DMA_ALIGN, sizeof(vmxnet3_dma));
  memset(this->dma, 0, sizeof(vmxnet3_dma));

  auto& queues = dma->queues;
  // setup tx queues
  queues.tx.cfg.desc_address = (uintptr_t) &dma->tx_desc;
  queues.tx.cfg.comp_address = (uintptr_t) &dma->tx_comp;
  queues.tx.cfg.num_desc     = vmxnet3::NUM_TX_DESC;
  queues.tx.cfg.num_comp     = VMXNET3_NUM_TX_COMP;
  queues.tx.cfg.intr_index   = 1;
  // temp rxq buffer storage
  memset(tx.buffers, 0, sizeof(tx.buffers));

  // setup rx queues
  for (int q = 0; q < NUM_RX_QUEUES; q++)
  {
    memset(rx[q].buffers, 0, sizeof(rx[q].buffers));
    rx[q].desc0 = &dma->rx0_desc[0];
    rx[q].desc1 = &dma->rx1_desc[0];
    rx[q].comp  = &dma->rx_comp[0];
    rx[q].index = q;

    auto& queue = queues.rx[q];
    queue.cfg.desc_address[0] = (uintptr_t) rx[q].desc0;
    queue.cfg.desc_address[1] = (uintptr_t) rx[q].desc1;
    queue.cfg.comp_address    = (uintptr_t) rx[q].comp;
    queue.cfg.num_desc[0]  = vmxnet3::NUM_RX_DESC;
    queue.cfg.num_desc[1]  = vmxnet3::NUM_RX_DESC;
    queue.cfg.num_comp     = VMXNET3_NUM_RX_COMP;
    queue.cfg.driver_data_len = sizeof(vmxnet3_rx_desc)
                          + 2 * sizeof(vmxnet3_rx_desc);
    queue.cfg.intr_index = 2 + q;
  }

  auto& shared = dma->shared;
  // setup shared physical area
  shared.magic = VMXNET3_REV1_MAGIC;
  shared.misc.guest_info.arch =
      (sizeof(void*) == 4) ? GOS_BITS_32_BITS : GOS_BITS_64_BITS;
  shared.misc.guest_info.type = GOS_TYPE_LINUX;
  shared.misc.version         = VMXNET3_VERSION_MAGIC;
  shared.misc.version_support     = 1;
  shared.misc.upt_version_support = 1;
  shared.misc.driver_data_address = (uintptr_t) &dma;
  shared.misc.queue_desc_address  = (uintptr_t) &dma->queues;
  shared.misc.driver_data_len     = sizeof(vmxnet3_dma);
  shared.misc.queue_desc_len      = sizeof(vmxnet3_queues);
  shared.misc.mtu = packet_len(); // 60-9000
  shared.misc.num_tx_queues  = 1;
  shared.misc.num_rx_queues  = NUM_RX_QUEUES;
  shared.interrupt.mask_mode = 0; // IMM_AUTO
  shared.interrupt.num_intrs = 2 + NUM_RX_QUEUES;
  shared.interrupt.event_intr_index = 0;
  memset(shared.interrupt.moderation_level, 0, VMXNET3_MAX_INTRS);
  shared.interrupt.control   = 0x1; // disable all
  shared.rx_filter.mode =
      VMXNET3_RXM_UCAST | VMXNET3_RXM_BCAST | VMXNET3_RXM_ALL_MULTI;

  // location of shared area to device
  uintptr_t shabus = (uintptr_t) &shared;
  mmio_write32(this->iobase + 0x10, shabus); // shared low
  mmio_write32(this->iobase + 0x18, 0x0);    // shared high

  // activate device
  int status = command(VMXNET3_CMD_ACTIVATE_DEV);
  if (status) {
    assert(0 && "Failed to activate device");
  }

  // initialize and fill RX queue...
  for (int q = 0; q < NUM_RX_QUEUES; q++)
  {
    refill(rx[q]);
  }

  // deferred transmit
  this->deferred_irq = IRQ_manager::get().get_free_irq();
  IRQ_manager::get().subscribe(this->deferred_irq, handle_deferred);

  // enable interrupts
  enable_intr(0);
  enable_intr(1);
  for (int q = 0; q < NUM_RX_QUEUES; q++)
      enable_intr(2 + q);
}

uint32_t vmxnet3::command(uint32_t cmd)
{
  mmio_write32(this->iobase + VMXNET3_VD_CMD, cmd);
  return mmio_read32(this->iobase + VMXNET3_VD_CMD);
}

bool vmxnet3::check_version()
{
  uint32_t maj_ver = mmio_read32(this->iobase + 0x0);
  uint32_t min_ver = mmio_read32(this->iobase + 0x8);
  INFO("vmxnet3", "Version %d.%d", maj_ver, min_ver);

  // select version we support
  mmio_write32(this->iobase + 0x0, 0x1);
  mmio_write32(this->iobase + 0x8, 0x1);
  return true;
}
uint16_t vmxnet3::check_link()
{
  auto state = command(VMXNET3_CMD_GET_LINK);
  bool link_up  = state & 1;
  if (link_up)
      return state >> 16;
  else
      return 0;
}
bool vmxnet3::reset()
{
  auto res = command(VMXNET3_CMD_RESET_DEV);
  return res == 0;
}

void vmxnet3::retrieve_hwaddr()
{
  struct {
    uint32_t lo;
    uint32_t hi;
  } mac;
  mac.lo = mmio_read32(this->iobase + VMXNET3_VD_MAC_LO);
  mac.hi = mmio_read32(this->iobase + VMXNET3_VD_MAC_HI);
  // ETH_ALEN = 6
  memcpy(&this->hw_addr, &mac, sizeof(hw_addr));
  INFO2("MAC address: %s", hw_addr.to_string().c_str());
}
void vmxnet3::set_hwaddr(MAC::Addr& addr)
{
  struct {
    uint32_t lo;
    uint32_t hi;
  } mac {0, 0};
  // ETH_ALEN = 6
  memcpy(&mac, &addr, sizeof(addr));

  mmio_write32(this->iobase + VMXNET3_VD_MAC_LO, mac.lo);
  mmio_write32(this->iobase + VMXNET3_VD_MAC_HI, mac.hi);
}
void vmxnet3::enable_intr(uint8_t idx) noexcept
{
#define VMXNET3_PT_IMR 0x0
  mmio_write32(this->ptbase + VMXNET3_PT_IMR + idx * 8, 0);
}
void vmxnet3::disable_intr(uint8_t idx) noexcept
{
  mmio_write32(this->ptbase + VMXNET3_PT_IMR + idx * 8, 1);
}

#define VMXNET3_PT_TXPROD  0x600
#define VMXNET3_PT_RXPROD1 0x800
#define VMXNET3_PT_RXPROD2 0xa00

#define VMXNET3_RXF_GEN  0x80000000UL
#define VMXNET3_RXCF_GEN 0x80000000UL
#define VMXNET3_TXF_GEN  0x00004000UL

void vmxnet3::refill(rxring_state& rxq)
{
  bool added_buffers = false;
  int  old_value = rxq.producers;

  while (rxq.prod_count < VMXNET3_RX_FILL
      && bufstore().available() != 0)
  {
    size_t i = rxq.producers % vmxnet3::NUM_RX_DESC;
    const uint32_t generation =
        (rxq.producers & vmxnet3::NUM_RX_DESC) ? 0 : VMXNET3_RXF_GEN;

    // get a pointer to packet data
    auto* pkt_data = bufstore().get_buffer().addr;
    rxq.buffers[i] = &pkt_data[sizeof(net::Packet)];

    // assign rx descriptor
    auto& desc = rxq.desc0[i];
    desc.address = (uintptr_t) rxq.buffers[i];
    desc.flags   = packet_len() | generation;
    rxq.prod_count++;
    rxq.producers++;

    added_buffers = true;
  }
  if (added_buffers && old_value != rxq.producers) {
    // send count to NIC
    mmio_write32(this->ptbase + VMXNET3_PT_RXPROD1 + 0x200 * rxq.index,
                 rxq.producers % vmxnet3::NUM_RX_DESC);
  }
}

net::Packet_ptr
vmxnet3::recv_packet(uint8_t* data, uint16_t size)
{
  auto* ptr = (net::Packet*) (data - sizeof(net::Packet));
  new (ptr) net::Packet(frame_offset_device(), size - frame_offset_device(), frame_offset_device() + packet_len() , &bufstore());
  return net::Packet_ptr(ptr);
}
net::Packet_ptr
vmxnet3::create_packet(int link_offset)
{
  auto buffer = bufstore().get_buffer();
  auto* ptr = (net::Packet*) buffer.addr;
  new (ptr) net::Packet(
        frame_offset_device() + link_offset, 
        0, 
        frame_offset_device() + packet_len(), 
        buffer.bufstore);
  return net::Packet_ptr(ptr);
}

void vmxnet3::msix_evt_handler()
{
  uint32_t evts = dma->shared.ecr;
  if (evts == 0) return;
  // ack all events
  mmio_write32(this->iobase + VMXNET3_VD_ECR, evts);

  printf("[vmxnet3] events: %#x\n", evts);
}
void vmxnet3::msix_xmit_handler()
{
  while (true)
  {
    uint32_t idx = tx.consumers % VMXNET3_NUM_TX_COMP;
    uint32_t gen = (tx.consumers & VMXNET3_NUM_TX_COMP) ? 0 : VMXNET3_TXCF_GEN;

    auto& comp = dma->tx_comp[idx];
    if (gen != (comp.flags & VMXNET3_TXCF_GEN)) break;

    tx.consumers++;

    int desc = comp.index % vmxnet3::NUM_TX_DESC;
    if (tx.buffers[desc] == nullptr) {
      printf("empty buffer? comp=%d, desc=%d\n", idx, desc);
      continue;
    }
    auto* packet = (net::Packet*) (tx.buffers[desc] - sizeof(net::Packet));
    packet->~Packet(); // call destructor on Packet to release it
    tx.buffers[desc] = nullptr;
  }
  // try to send sendq first
  if (this->can_transmit() && sendq != nullptr) {
    this->transmit(std::move(sendq));
  }
  // if we can still send more, message network stack
  if (this->can_transmit()) {
    transmit_queue_available_event_(tx_tokens_free());
  }
}
void vmxnet3::msix_recv_handler()
{
  while (true)
  {
    uint32_t idx = rx[0].consumers % VMXNET3_NUM_RX_COMP;
    uint32_t gen = (rx[0].consumers & VMXNET3_NUM_RX_COMP) ? 0 : VMXNET3_RXCF_GEN;

    auto& comp = dma->rx_comp[idx];
    // break when exiting this generation
    if (gen != (comp.flags & VMXNET3_RXCF_GEN)) break;
    rx[0].consumers++;
    rx[0].prod_count--;

    int desc = comp.index % vmxnet3::NUM_RX_DESC;
    // mask out length
    int len = comp.len & (VMXNET3_MAX_BUFFER_LEN-1);
    // get buffer and construct packet
    assert(rx[0].buffers[desc] != nullptr);
    auto packet = recv_packet(rx[0].buffers[desc], len);
    rx[0].buffers[desc] = nullptr;

    // handle_magic()
    Link::receive(std::move(packet));

    // emergency refill when really empty
    if (rx[0].prod_count < VMXNET3_RX_FILL / 2)
        refill(rx[0]);
  }
  refill(rx[0]);
}

void vmxnet3::transmit(net::Packet_ptr pckt_ptr)
{
  if (sendq == nullptr)
      sendq = std::move(pckt_ptr);
  else
      sendq->chain(std::move(pckt_ptr));
  // send as much as possible from sendq
  while (sendq != nullptr && can_transmit())
  {
    auto next = sendq->detach_tail();
    // transmit released buffer
    auto* packet = sendq.release();
    transmit_data(packet->buf(), packet->size());
    // next is the new sendq
    sendq = std::move(next);
  }
}
int  vmxnet3::tx_tokens_free() const noexcept
{
  return VMXNET3_TX_FILL - (tx.producers - tx.consumers);
}
bool vmxnet3::can_transmit() const noexcept
{
  return tx.producers - tx.consumers < VMXNET3_TX_FILL;
}

void vmxnet3::transmit_data(uint8_t* data, uint16_t data_length)
{
#define VMXNET3_TXF_EOP 0x000001000UL
#define VMXNET3_TXF_CQ  0x000002000UL
  auto idx = tx.producers % vmxnet3::NUM_TX_DESC;
  auto gen = (tx.producers & vmxnet3::NUM_TX_DESC) ? 0 : VMXNET3_TXF_GEN;
  tx.producers++;

  assert(tx.buffers[idx] == nullptr);
  tx.buffers[idx] = data;

  auto& desc = dma->tx_desc[idx];
  desc.address  = (uintptr_t) tx.buffers[idx];
  desc.flags[0] = gen | data_length;
  desc.flags[1] = VMXNET3_TXF_CQ | VMXNET3_TXF_EOP;

  // delay dma message until we have written as much as possible
  if (!deferred_kick)
  {
    transmit_idx  = idx;
    deferred_kick = true;
    deferred_devs.push_back(this);
    IRQ_manager::get().register_irq(deferred_irq);
  }
}
void vmxnet3::handle_deferred()
{
  for (auto* dev : deferred_devs)
  {
    auto idx = dev->tx.producers % vmxnet3::NUM_TX_DESC;
    if (idx != dev->transmit_idx)
    {
      //printf("idx: %d   t.idx: %d\n", idx, dev->transmit_idx);
      mmio_write32(dev->ptbase + VMXNET3_PT_TXPROD, idx);
    }
    else {
      printf("Avoided mmio write\n");
    }
    dev->deferred_kick = false;
  }
  deferred_devs.clear();
}

void vmxnet3::deactivate()
{
  assert(0);
}

void vmxnet3::move_to_this_cpu()
{
  bufstore().move_to_this_cpu();

  if (pcidev.has_msix())
  {
    for (size_t i = 0; i < irqs.size(); i++)
    {
      this->irqs[i] = IRQ_manager::get().get_free_irq();
      pcidev.rebalance_msix_vector(i, SMP::cpu_id(), IRQ_BASE + this->irqs[i]);
      IRQ_manager::get().subscribe(this->irqs[i], nullptr);
    }
  }
}

#include <kernel/pci_manager.hpp>
__attribute__((constructor))
static void register_func()
{
  PCI_manager::register_driver<hw::Nic>(hw::PCI_Device::VENDOR_VMWARE, PRODUCT_ID, &vmxnet3::new_instance);
}
