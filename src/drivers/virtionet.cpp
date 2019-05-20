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

//#define VNET_DEBUG
//#define VNET_DEBUG_RX
//#define VNET_DEBUG_TX

#ifdef VNET_DEBUG
#define VDBG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define VDBG(fmt, ...) /* fmt */
#endif

#ifdef VNET_DEBUG_RX
#define VDBG_RX(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define VDBG_RX(fmt, ...) /* fmt */
#endif

#ifdef VNET_DEBUG_TX
#define VDBG_TX(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define VDBG_TX(fmt, ...) /* fmt */
#endif

#include "virtionet.hpp"
#include <kernel/events.hpp>
#include <malloc.h>
#include <cstring>

//#define NO_DEFERRED_KICK
#ifndef NO_DEFERRED_KICK
#include <smp>
struct alignas(SMP_ALIGN) smp_deferred_kick
{
  std::vector<VirtioNet*> devs;
  uint8_t irq;
};
static std::array<smp_deferred_kick, SMP_MAX_CORES> deferred_devs;
#endif

using namespace net;

void VirtioNet::get_config() {
  Virtio::get_config(&_conf, _config_length);
}
#define VNET_TOT_BUFFERS() (48 + (queue_size(0) + queue_size(1)) / 2)

VirtioNet::VirtioNet(hw::PCI_Device& d, const uint16_t /*mtu*/)
  : Virtio(d),
    Link(Link_protocol{{this, &VirtioNet::transmit}, mac()}),
    m_pcidev(d),
    bufstore_{VNET_TOT_BUFFERS(), 2048 /* half-page buffers */},

    stat_sendq_max_{Statman::get().create(Stat::UINT64,
                device_name() + ".sendq_max").get_uint64()},
    stat_sendq_now_{Statman::get().create(Stat::UINT64,
                device_name() + ".sendq_now").get_uint64()},
    stat_sendq_limit_dropped_{Statman::get().create(Stat::UINT64,
                device_name() + ".sendq_dropped").get_uint64()},
    stat_rx_refill_dropped_{Statman::get().create(Stat::UINT64,
                device_name() + ".rx_refill_dropped").get_uint64()},
    stat_bytes_rx_total_{Statman::get().create(Stat::UINT64,
                device_name() + ".stat_rx_total_bytes").get_uint64()},
    stat_bytes_tx_total_{Statman::get().create(Stat::UINT64,
                device_name() + ".stat_tx_total_bytes").get_uint64()},
    stat_packets_rx_total_{Statman::get().create(Stat::UINT64,
                device_name() + ".stat_rx_total_packets").get_uint64()},
    stat_packets_tx_total_{Statman::get().create(Stat::UINT64,
                device_name() + ".stat_tx_total_packets").get_uint64()}

{
  INFO("VirtioNet", "Driver initializing");
#undef VNET_TOT_BUFFERS

  uint32_t needed_features = 0
    | (1 << VIRTIO_NET_F_MAC)
    | (1 << VIRTIO_NET_F_STATUS)
    ;//| (1 << VIRTIO_NET_F_MRG_RXBUF); //Merge RX Buffers (Everything i 1 buffer)
  uint32_t wanted_features = needed_features;
  negotiate_features(wanted_features);


  CHECK ((features() & needed_features) == needed_features,
         "Negotiated needed features");

  CHECK ((features() & wanted_features) == wanted_features,
         "Negotiated wanted features");

  CHECK(features() & (1 << VIRTIO_NET_F_CSUM),
        "Device handles packets w. partial checksum");

  CHECK(features() & (1 << VIRTIO_NET_F_GUEST_CSUM),
        "Guest handles packets w. partial checksum");

  CHECK(features() & (1 << VIRTIO_NET_F_CTRL_VQ),
        "There's a control queue");

  CHECK(features() & (1 << VIRTIO_F_ANY_LAYOUT),
        "Queue can handle any header/data layout");

  CHECK(features() & (1 << VIRTIO_F_RING_INDIRECT_DESC),
        "We can use indirect descriptors");

  CHECK(features() & (1 << VIRTIO_F_RING_EVENT_IDX),
        "There's a Ring Event Index to use");

  CHECK(features() & (1 << VIRTIO_NET_F_MQ),
        "There are multiple queue pairs");

  if (features() & (1 << VIRTIO_NET_F_MQ))
    printf("\t\t* max_virtqueue_pairs: 0x%x \n",_conf.max_virtq_pairs);

  CHECK(features() & (1 << VIRTIO_NET_F_MRG_RXBUF),
        "Merge RX buffers");


  /** RX que is 0, TX Queue is 1 - Virtio Std. §5.1.2  */
  new (&rx_q) Virtio::Queue(device_name() + ".rx_q", queue_size(0),0,iobase());
  new (&tx_q) Virtio::Queue(device_name() + ".tx_q", queue_size(1),1,iobase());
  new (&ctrl_q) Virtio::Queue(device_name() + ".ctl_q", queue_size(2),2,iobase());

  // Step 1 - Initialize RX/TX queues
  auto success = assign_queue(0, rx_q.queue_desc());
  CHECKSERT(success, "RX queue (%u) assigned (%p) to device",
        rx_q.size(), rx_q.queue_desc());

  success = assign_queue(1, tx_q.queue_desc());
  CHECKSERT(success, "TX queue (%u) assigned (%p) to device",
        tx_q.size(), tx_q.queue_desc());

  // Step 2 - Initialize Ctrl-queue if it exists
  if (features() & (1 << VIRTIO_NET_F_CTRL_VQ)) {
    success = assign_queue(2, tx_q.queue_desc());
    CHECKSERT(success, "CTRL queue (%u) assigned (%p) to device",
          ctrl_q.size(), ctrl_q.queue_desc());
  }

  // Step 3 - Fill receive queue with buffers
  // DEBUG: Disable
  INFO("VirtioNet", "Adding %u receive buffers of size %u",
       rx_q.size() / 2, (uint32_t) bufstore().bufsize());

  for (int i = 0; i < rx_q.size() / 2; i++) {
      add_receive_buffer(bufstore().get_buffer());
  }

  // Step 4 - If there are many queues, we should negotiate the number.
  // Set config length, based on whether there are multiple queues
  if (features() & (1 << VIRTIO_NET_F_MQ))
    _config_length = sizeof(config);
  else
    _config_length = sizeof(config) - sizeof(uint16_t);
  // @todo: Specify how many queues we're going to use.

  // Step 5 - get the mac address (we're demanding this feature)
  // Step 6 - get the status - demanding this as well.
  // Getting the MAC + status
  get_config();

  CHECK(_conf.mac.major > 0, "Valid Mac address: %s",
        _conf.mac.str().c_str());


  // Step 7 - 9 - GSO: @todo Not using GSO features yet.

  // Signal setup complete.
  setup_complete((features() & needed_features) == needed_features);
  CHECK((features() & needed_features) == needed_features, "Signalled driver OK");

  // Hook up interrupts
  if (has_msix())
  {
    assert(get_msix_vectors() >= 3);
    auto& irqs = this->get_irqs();
    // update BSP IDT
    Events::get().subscribe(irqs[0], {this, &VirtioNet::msix_recv_handler});
    Events::get().subscribe(irqs[1], {this, &VirtioNet::msix_xmit_handler});
    Events::get().subscribe(irqs[2], {this, &VirtioNet::msix_conf_handler});
  }
  else
  {
    auto irq = Virtio::get_legacy_irq();
    Events::get().subscribe(irq, {this, &VirtioNet::legacy_handler});
  }

#ifndef NO_DEFERRED_KICK
  static bool init_deferred = false;
  if (!init_deferred) {
    init_deferred = true;
    auto defirq = Events::get().subscribe(handle_deferred_devices);
    PER_CPU(deferred_devs).irq = defirq;
  }
#endif

  CHECK(this->link_up(), "Link up");
  // Done
  if (this->link_up()) {
    rx_q.kick();
  }
}

bool VirtioNet::link_up() const noexcept
{
  return _conf.status & 1;
}

void VirtioNet::msix_conf_handler()
{
  VDBG("\t <VirtioNet> Configuration change:\n");

  // Getting the MAC + status
  VDBG("\t    Old status: 0x%x\n",_conf.status);
  get_config();
  VDBG("\t    New status: 0x%x \n",_conf.status);
}
void VirtioNet::msix_recv_handler()
{
  auto rx = stat_packets_rx_total_;
  rx_q.disable_interrupts();
  // handle incoming packets as long as bufstore has available buffers
  int max = 128;
  while (rx_q.new_incoming() && max-- > 0)
  {
    auto res = rx_q.dequeue();
    VDBG_RX("[virtionet] Recv %u bytes\n", (uint32_t) res.size());
    auto pckt = recv_packet(res.data(), res.size());

    // Stat increase packets received
    stat_packets_rx_total_++;
    stat_bytes_rx_total_ += pckt->size();

    Link::receive(std::move(pckt));

    // Requeue a new buffer unless threshold is reached
    if (not Nic::buffers_still_available(bufstore().buffers_in_use()))
    {
      stat_rx_refill_dropped_++;
      break;
    }
    add_receive_buffer(bufstore().get_buffer());
  }
  rx_q.enable_interrupts();
  if (rx != stat_packets_rx_total_) rx_q.kick();
}
void VirtioNet::msix_xmit_handler()
{
  int dequeued_tx = 0;
  tx_q.disable_interrupts();
  // Do one TX-packet
  while (tx_q.new_incoming())
  {
    auto res = tx_q.dequeue();
    assert(res.data() != nullptr);
    // get packet offset, and call placement Packet deleter directly
    net::Packet::operator delete(res.data() - sizeof(net::Packet));
    dequeued_tx++;
  }
  tx_q.enable_interrupts();

  // If we have a transmit queue, eat from it, otherwise let the stack know we
  // have increased transmit capacity
  if (dequeued_tx > 0)
  {
    VDBG_TX("[virtionet] %d transmitted\n", dequeued_tx);

    // transmit as much as possible from the buffer
    if (! sendq.empty()) {
      transmit(nullptr);
    }

    // If we now emptied the buffer, offer packets to stack
    if (sendq.empty() && tx_q.num_free() > 1) {
      transmit_queue_available_event(tx_q.num_free() / 2);
    }
  }
}

void VirtioNet::legacy_handler()
{
  msix_recv_handler();
  msix_xmit_handler();
}

void VirtioNet::add_receive_buffer(uint8_t* pkt)
{
  assert(pkt >= (uint8_t*) 0x1000);
  // offset pointer to virtionet header
  auto* vnet = pkt + sizeof(Packet);

  Token token1 {{vnet, sizeof(virtio_net_hdr)}, Token::IN };
  Token token2 {{vnet + sizeof(virtio_net_hdr), max_packet_len()}, Token::IN };

  std::array<Token, 2> tokens {{ token1, token2 }};
  rx_q.enqueue(tokens);
}

net::Packet_ptr
VirtioNet::recv_packet(uint8_t* data, uint16_t size)
{
  auto* ptr = (net::Packet*) (data - sizeof(net::Packet));

  new (ptr) net::Packet(
      sizeof(virtio_net_hdr),
      size - sizeof(virtio_net_hdr),
      size,
      &bufstore());

  return net::Packet_ptr(ptr);
}

net::Packet_ptr
VirtioNet::create_packet(int link_offset)
{
  auto* ptr = (net::Packet*) bufstore().get_buffer();

  new (ptr) net::Packet(
        sizeof(virtio_net_hdr) + link_offset,
        0,
        sizeof(virtio_net_hdr) + frame_offset_link() + MTU(),
        &bufstore());

  return net::Packet_ptr(ptr);
}

void VirtioNet::transmit(net::Packet_ptr pckt)
{
  while (pckt != nullptr) {
    if (not Nic::sendq_still_available(sendq.size())) {
      stat_sendq_limit_dropped_ += pckt->chain_length();
      break;
    }
    VDBG_TX("[virtionet] tx: Transmitting %#zu sized packet \n",
            pckt->size());
    auto tail = pckt->detach_tail();
    sendq.emplace_back(std::move(pckt));
    pckt = std::move(tail);
  }

  // Update sendq stats
  stat_sendq_now_ = sendq.size();
  if (sendq.size() > stat_sendq_max_)
    stat_sendq_max_ = sendq.size();

  auto tx = this->stat_packets_tx_total_;

  VDBG_TX("[virtionet] tx: packets in send queue %#zu\n",
          sendq.size());

  // Transmit all we can directly
  while (tx_q.num_free() > 1 and !sendq.empty())
  {
    VDBG_TX("[virtionet] tx: %u tokens left in TX ring \n",
            tx_q.num_free());

    auto* next = sendq.front().release();
    sendq.pop_front();
    enqueue_tx(next);

    // Increase TX-stats
    stat_packets_tx_total_++;
    stat_bytes_tx_total_ += next->size();
    stat_packets_tx_total_++;
  }

  VDBG_TX("[virtionet] tx: packet enqueued\n");

  if (tx != this->stat_packets_tx_total_) {
#ifdef NO_DEFERRED_KICK
    tx_q.kick();
#else
    if (!deferred_kick) {
      deferred_kick = true;
      PER_CPU(deferred_devs).devs.push_back(this);
      Events::get().trigger_event(PER_CPU(deferred_devs).irq);
    }
#endif
  }
}

void VirtioNet::enqueue_tx(net::Packet* pckt)
{
  Expects(pckt->layer_begin() == pckt->buf() + sizeof(virtio_net_hdr));
  auto* hdr = pckt->buf();
  memset(hdr, 0, sizeof(virtio_net_hdr));
  VDBG_TX("[virtionet] tx: Transmit %u bytes\n", (uint32_t) pckt->size());

  Token token1 {{ hdr, sizeof(virtio_net_hdr)}, Token::OUT };
  Token token2 {{ pckt->layer_begin(), pckt->size()}, Token::OUT };

  std::array<Token, 2> tokens {{ token1, token2 }};

  // Enqueue scatterlist, 2 pieces readable, 0 writable.
  tx_q.enqueue(tokens);
}

void VirtioNet::handle_deferred_devices()
{
#ifndef NO_DEFERRED_KICK
  for (auto* dev : PER_CPU(deferred_devs).devs)
  if (dev->deferred_kick)
  {
    dev->deferred_kick = false;
    // kick transmitq
    dev->tx_q.kick();
  }
  PER_CPU(deferred_devs).devs.clear();
#endif
}

void VirtioNet::poll()
{
  msix_recv_handler();
  msix_xmit_handler();
  // flush transmit_q immediately
  if (this->deferred_kick)
  {
    this->deferred_kick = false;
    this->tx_q.enable_interrupts();
    this->tx_q.kick();
  }
}

void VirtioNet::deactivate()
{
  VDBG("[virtionet] Disabling device\n");
  /// disable interrupts on virtio queues
  rx_q.disable_interrupts();
  tx_q.disable_interrupts();
  ctrl_q.disable_interrupts();

  // reset device
  this->Virtio::reset();
}

void VirtioNet::move_to_this_cpu()
{
  INFO("VirtioNet", "Moving to CPU %d", SMP::cpu_id());
  // update CPU id in bufferstore
  bufstore().move_to_this_cpu();
  // virtio IRQ balancing
  this->Virtio::move_to_this_cpu();
  // reset the IRQ handlers on this CPU
  auto& irqs = this->Virtio::get_irqs();
  Events::get().subscribe(irqs[0], {this, &VirtioNet::msix_recv_handler});
  Events::get().subscribe(irqs[1], {this, &VirtioNet::msix_xmit_handler});
  Events::get().subscribe(irqs[2], {this, &VirtioNet::msix_conf_handler});
#ifndef NO_DEFERRED_KICK
  // update deferred kick IRQ
  auto defirq = Events::get().subscribe(handle_deferred_devices);
  PER_CPU(deferred_devs).irq = defirq;
#endif
}

#include <hw/pci_manager.hpp>

/** Register VirtioNet's driver factory at the PCI_manager */
__attribute__((constructor))
void autoreg_virtionet() {
  hw::PCI_manager::register_nic(PCI::VENDOR_VIRTIO, 0x1000, &VirtioNet::new_instance);
}
