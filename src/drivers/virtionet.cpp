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

#define PRINT_INFO
//#define DEBUG
//#define DEBUG2

#include "virtionet.hpp"
#include <kernel/irq_manager.hpp>
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

VirtioNet::VirtioNet(hw::PCI_Device& d)
  : Virtio(d),
    Link(Link_protocol{{this, &VirtioNet::transmit}, mac()},
        256u, 2048 /* 256x half-page buffers */),
    packets_rx_{Statman::get().create(Stat::UINT64, device_name() + ".packets_rx").get_uint64()},
    packets_tx_{Statman::get().create(Stat::UINT64, device_name() + ".packets_tx").get_uint64()}
{
  INFO("VirtioNet", "Driver initializing");

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
  new (&rx_q) Virtio::Queue(queue_size(0),0,iobase());
  new (&tx_q) Virtio::Queue(queue_size(1),1,iobase());
  new (&ctrl_q) Virtio::Queue(queue_size(2),2,iobase());

  // Step 1 - Initialize RX/TX queues
  auto success = assign_queue(0, (uint32_t)rx_q.queue_desc());
  CHECK(success, "RX queue (%u) assigned (0x%x) to device",
        rx_q.size(), (uint32_t)rx_q.queue_desc());

  success = assign_queue(1, (uint32_t)tx_q.queue_desc());
  CHECK(success, "TX queue (%u) assigned (0x%x) to device",
        tx_q.size(), (uint32_t)tx_q.queue_desc());

  // Step 2 - Initialize Ctrl-queue if it exists
  if (features() & (1 << VIRTIO_NET_F_CTRL_VQ)) {
    success = assign_queue(2, (uint32_t)tx_q.queue_desc());
    CHECK(success, "CTRL queue (%u) assigned (0x%x) to device",
          ctrl_q.size(), (uint32_t)ctrl_q.queue_desc());
  }

  // Step 3 - Fill receive queue with buffers
  // DEBUG: Disable
  INFO("VirtioNet", "Adding %u receive buffers of size %u",
       rx_q.size() / 2, bufstore().bufsize());

  for (int i = 0; i < rx_q.size() / 2; i++)
      add_receive_buffer(bufstore().get_buffer().addr);

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
    IRQ_manager::get().subscribe(irqs[0], {this, &VirtioNet::msix_recv_handler});
    IRQ_manager::get().subscribe(irqs[1], {this, &VirtioNet::msix_xmit_handler});
    IRQ_manager::get().subscribe(irqs[2], {this, &VirtioNet::msix_conf_handler});
  }
  else
  {
    assert(0 && "Legacy IRQs not supported");
  }

#ifndef NO_DEFERRED_KICK
  static bool init_deferred = false;
  if (!init_deferred) {
    init_deferred = true;
    auto defirq = IRQ_manager::get().get_free_irq();
    PER_CPU(deferred_devs).irq = defirq;
    IRQ_manager::get().subscribe(defirq, handle_deferred_devices);
  }
#endif

  // Done
  INFO("VirtioNet", "Driver initialization complete");
  CHECK(_conf.status & 1, "Link up\n");
  rx_q.kick();
}

void VirtioNet::msix_conf_handler()
{
  debug("\t <VirtioNet> Configuration change:\n");

  // Getting the MAC + status
  debug("\t             Old status: 0x%x\n",_conf.status);
  get_config();
  debug("\t             New status: 0x%x \n",_conf.status);
}
void VirtioNet::msix_recv_handler()
{
  int dequeued_rx = 0;
  rx_q.disable_interrupts();
  // handle incoming packets as long as bufstore has available buffers
  while (rx_q.new_incoming())
  {
    auto res = rx_q.dequeue();
    Link::receive( recv_packet(res.data(), res.size()) );

    dequeued_rx++;
    // Requeue a new buffer
    add_receive_buffer(bufstore().get_buffer().addr);

    // Stat increase packets received
    packets_rx_++;
  }
  rx_q.enable_interrupts();
  if (dequeued_rx)
    rx_q.kick();
}
void VirtioNet::msix_xmit_handler()
{
  bool dequeued_tx = false;
  tx_q.disable_interrupts();
  // Do one TX-packet
  while (tx_q.new_incoming())
  {
    auto res = tx_q.dequeue();

    // get packet offset, and call destructor
    auto* packet = (net::Packet*) (res.data() - sizeof(net::Packet));
    packet->~Packet(); // call destructor on Packet to release it
    dequeued_tx = true;
  }

  // If we have a transmit queue, eat from it, otherwise let the stack know we
  // have increased transmit capacity
  if (dequeued_tx)
  {
    debug("<VirtioNet>%i dequeued, transmitting backlog\n", dequeued_tx);

    // transmit as much as possible from the buffer
    if (transmit_queue_) {
      transmit(std::move(transmit_queue_));
    }

    // If we now emptied the buffer, offer packets to stack
    if (!transmit_queue_ && tx_q.num_free() > 1)
        transmit_queue_available_event_(tx_q.num_free() / 2);
  }
}

void VirtioNet::add_receive_buffer(uint8_t* pkt)
{
  // offset pointer to virtionet header
  auto* vnet = pkt + sizeof(Packet);

  Token token1 {{vnet, sizeof(virtio_net_hdr)}, Token::IN };
  Token token2 {{vnet + sizeof(virtio_net_hdr), packet_len()}, Token::IN };

  std::array<Token, 2> tokens {{ token1, token2 }};
  rx_q.enqueue(tokens);
}

net::Packet_ptr
VirtioNet::recv_packet(uint8_t* data, uint16_t size)
{
  auto* ptr = (net::Packet*) (data - sizeof(net::Packet));
#ifdef DEBUG
  assert(bufstore().is_from_pool((uint8_t*) ptr));
  assert(bufstore().is_buffer((uint8_t*) ptr));
#endif

  new (ptr) net::Packet(
      sizeof(virtio_net_hdr), 
      size - sizeof(virtio_net_hdr), 
      sizeof(virtio_net_hdr) + packet_len(), 
      &bufstore());

  return net::Packet_ptr(ptr);
}

net::Packet_ptr
VirtioNet::create_packet(int link_offset)
{
  auto buffer = bufstore().get_buffer();
  auto* ptr = (net::Packet*) buffer.addr;

  new (ptr) net::Packet(
        sizeof(virtio_net_hdr) + link_offset, 
        0, 
        sizeof(virtio_net_hdr) + packet_len(), 
        buffer.bufstore);

  return net::Packet_ptr(ptr);
}

void VirtioNet::add_to_tx_buffer(net::Packet_ptr pckt){
  if (transmit_queue_)
    transmit_queue_->chain(std::move(pckt));
  else
    transmit_queue_ = std::move(pckt);

#ifdef DEBUG
  size_t chain_length = 1;
  auto* next = transmit_queue_->tail();
  while (next) {
    chain_length++;
    next = next->tail();
  }
#endif

  debug("Buffering, %i packets chained \n", chain_length);
}

void VirtioNet::transmit(net::Packet_ptr pckt) {
  /** @note We have to send a virtio header first, then the packet.

      From Virtio std. §5.1.6.6:
      "When using legacy interfaces, transitional drivers which have not
      negotiated VIRTIO_F_ANY_LAYOUT MUST use a single descriptor for the struct
      virtio_net_hdr on both transmit and receive, with the network data in the
      following descriptors."

      VirtualBox *does not* accept ANY_LAYOUT, while Qemu does, so this is to
      support VirtualBox
  */
  int transmitted = 0;
  net::Packet_ptr tail = std::move(pckt);

  // Transmit all we can directly
  while (tx_q.num_free() and tail != nullptr)
  {
    debug("%i tokens left in TX queue \n", tx_q.num_free());
    // next in line
    auto next = tail->detach_tail();
    // write data to network
    // explicitly release the data to prevent destructor being called
    enqueue(tail.release());
    tail = std::move(next);
    transmitted++;
    // Stat increase packets transmitted
    packets_tx_++;
  }

  if (LIKELY(transmitted)) {
#ifdef NO_DEFERRED_KICK
    tx_q.enable_interrupts();
    tx_q.kick();
#else
    begin_deferred_kick();
#endif
  }

  // Buffer the rest
  if (UNLIKELY(tail)) {
    debug("Buffering remaining packets..\n");
    add_to_tx_buffer(std::move(tail));
  }
}

void VirtioNet::enqueue(net::Packet* pckt)
{
  Expects(pckt->layer_begin() == pckt->buf() + sizeof(virtio_net_hdr));
  auto* hdr = pckt->buf();

  Token token1 {{ hdr, sizeof(virtio_net_hdr)}, Token::OUT };
  Token token2 {{ pckt->layer_begin(), pckt->size()}, Token::OUT };

  std::array<Token, 2> tokens {{ token1, token2 }};

  // Enqueue scatterlist, 2 pieces readable, 0 writable.
  tx_q.enqueue(tokens);
}

void VirtioNet::begin_deferred_kick()
{
#ifndef NO_DEFERRED_KICK
  if (!deferred_kick) {
    deferred_kick = true;
    PER_CPU(deferred_devs).devs.push_back(this);
    IRQ_manager::get().register_irq(PER_CPU(deferred_devs).irq);
  }
#endif
}
void VirtioNet::handle_deferred_devices()
{
#ifndef NO_DEFERRED_KICK
  for (auto* dev : PER_CPU(deferred_devs).devs)
  if (dev->deferred_kick)
  {
    dev->deferred_kick = false;
    // kick transmitq
    dev->tx_q.enable_interrupts();
    dev->tx_q.kick();
  }
  PER_CPU(deferred_devs).devs.clear();
#endif
}

void VirtioNet::deactivate()
{
  /// disable interrupts on virtio queues
  rx_q.disable_interrupts();
  tx_q.disable_interrupts();
  ctrl_q.disable_interrupts();

  /// mask off MSI-X vectors
  if (has_msix())
      deactivate_msix();
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
  IRQ_manager::get().subscribe(irqs[0], {this, &VirtioNet::msix_recv_handler});
  IRQ_manager::get().subscribe(irqs[1], {this, &VirtioNet::msix_xmit_handler});
  IRQ_manager::get().subscribe(irqs[2], {this, &VirtioNet::msix_conf_handler});
#ifndef NO_DEFERRED_KICK
  // update deferred kick IRQ
  auto defirq = IRQ_manager::get().get_free_irq();
  PER_CPU(deferred_devs).irq = defirq;
  IRQ_manager::get().subscribe(defirq, handle_deferred_devices);
#endif
}

#include <kernel/pci_manager.hpp>

/** Register VirtioNet's driver factory at the PCI_manager */
static struct Autoreg_virtionet {
  Autoreg_virtionet() {
    PCI_manager::register_driver<hw::Nic>(hw::PCI_Device::VENDOR_VIRTIO, 0x1000, &VirtioNet::new_instance);
  }
} autoreg_virtionet;
