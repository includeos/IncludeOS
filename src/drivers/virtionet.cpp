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
#define DEBUG // Allow debuging
#define DEBUG2

#include "virtionet.hpp"
#include <net/packet.hpp>
#include <kernel/irq_manager.hpp>
#include <kernel/syscalls.hpp>
#include <hw/pci.hpp>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
//#define NO_DEFERRED_KICK
#ifndef NO_DEFERRED_KICK
static std::vector<VirtioNet*> deferred_devices;
static uint8_t deferred_intr;
#endif

using namespace net;
constexpr VirtioNet::virtio_net_hdr VirtioNet::empty_header;

const char* VirtioNet::name() const { return "VirtioNet Driver"; }

void VirtioNet::get_config() {
  Virtio::get_config(&_conf, _config_length);
}

void VirtioNet::drop(Packet_ptr){
  debug("<VirtioNet->link-layer> No delegate. DROP!\n");
}

VirtioNet::VirtioNet(hw::PCI_Device& d)
  : Virtio(d),
    Link(Link_protocol{{this, &VirtioNet::transmit}, mac()}, 2048, sizeof(net::Packet) + MTU()),
    packets_rx_{Statman::get().create(Stat::UINT64, ifname() + ".packets_rx").get_uint64()},
    packets_tx_{Statman::get().create(Stat::UINT64, ifname() + ".packets_tx").get_uint64()},
    /** RX que is 0, TX Queue is 1 - Virtio Std. §5.1.2  */
    rx_q(queue_size(0),0,iobase()), tx_q(queue_size(1),1,iobase()),
    ctrl_q(queue_size(2),2,iobase())
{
  INFO("VirtioNet", "Driver initializing");
  // this must be true, otherwise packets will be created incorrectly
  assert(sizeof(virtio_net_hdr) <= sizeof(Packet));

  uint32_t needed_features = 0
    | (1 << VIRTIO_NET_F_MAC)
    | (1 << VIRTIO_NET_F_STATUS);
  //| (1 << VIRTIO_NET_F_MRG_RXBUF); //Merge RX Buffers (Everything i 1 buffer)
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


  // Step 1 - Initialize RX/TX queues
  auto success = assign_queue(0, (uint32_t)rx_q.queue_desc());
  CHECK(success, "RX queue assigned (0x%x) to device",
        (uint32_t)rx_q.queue_desc());

  success = assign_queue(1, (uint32_t)tx_q.queue_desc());
  CHECK(success, "TX queue assigned (0x%x) to device",
        (uint32_t)tx_q.queue_desc());

  // Step 2 - Initialize Ctrl-queue if it exists
  if (features() & (1 << VIRTIO_NET_F_CTRL_VQ)) {
    success = assign_queue(2, (uint32_t)tx_q.queue_desc());
    CHECK(success, "CTRL queue assigned (0x%x) to device",
          (uint32_t)ctrl_q.queue_desc());
  }

  // Step 3 - Fill receive queue with buffers
  // DEBUG: Disable
  INFO("VirtioNet", "Adding %i receive buffers of size %i",
       rx_q.size() / 2, bufsize());

  for (int i = 0; i < rx_q.size() / 2; i++) add_receive_buffer();

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
  if (is_msix())
  {
    // for now use service queues, otherwise stress test fails
    auto recv_del(delegate<void()>{this, &VirtioNet::msix_recv_handler});
    auto xmit_del(delegate<void()>{this, &VirtioNet::msix_xmit_handler});
    auto conf_del(delegate<void()>{this, &VirtioNet::msix_conf_handler});

    // update BSP IDT
    IRQ_manager::get().subscribe(irq() + 0, recv_del);
    IRQ_manager::get().subscribe(irq() + 1, xmit_del);
    IRQ_manager::get().subscribe(irq() + 2, conf_del);
  }
  else
  {
    // legacy PCI interrupt
    auto del(delegate<void()>{this, &VirtioNet::irq_handler});
    IRQ_manager::get().subscribe(irq(),del);
  }

#ifndef NO_DEFERRED_KICK
  static bool init_deferred = false;
  if (!init_deferred) {
    init_deferred = true;
    deferred_intr = IRQ_manager::get().get_next_msix_irq();
    IRQ_manager::get().subscribe(deferred_intr, handle_deferred_devices);
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
  bool dequeued_rx = false;
  rx_q.disable_interrupts();
  // handle incoming packets as long as bufstore has available buffers
  while (rx_q.new_incoming() && bufstore().available() > 1) {

    auto res = rx_q.dequeue();

    auto pckt_ptr = recv_packet(res.data(), res.size());
    Link::receive(std::move(pckt_ptr));

    // Requeue a new buffer
    add_receive_buffer();

    dequeued_rx = true;

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
  while (tx_q.new_incoming()) {
    // FIXME Unfortunately dequeue is not working here
    // I am guessing that Linux is eating the buffers?
    //auto res =
    tx_q.dequeue();

    // release the data back to pool
    auto data = tx_ringq.front();
    tx_ringq.pop_front();
    bufstore().release(data);

    dequeued_tx = true;
  }
  tx_q.enable_interrupts();

  // If we have a transmit queue, eat from it, otherwise let the stack know we
  // have increased transmit capacity
  if (dequeued_tx) {
    debug("<VirtioNet>%i dequeued, transmitting backlog\n", dequeued_tx);

    // transmit as much as possible from the buffer
    if (transmit_queue_){
      transmit(std::move(transmit_queue_));
    }

    // If we now emptied the buffer, offer packets to stack
    if (!transmit_queue_ && tx_q.num_free() > 1)
        transmit_queue_available_event_(tx_q.num_free() / 2);
  }
}

void VirtioNet::irq_handler() {
  // Virtio Std. § 4.1.5.5, steps 1-3

  // Step 1. read ISR
  unsigned char isr = hw::inp(iobase() + VIRTIO_PCI_ISR);

  // Step 2. A) - one of the queues have changed
  if (isr & 1) {
    // This now means service RX & TX interchangeably
    // We need a zipper-solution; we can't receive n packets before sending
    // anything - that's unfair.
    service_queues();
  }

  // Step 2. B)
  if (isr & 2) {
    debug("\t <VirtioNet> Configuration change:\n");

    // Getting the MAC + status
    debug("\t             Old status: 0x%x\n",_conf.status);
    get_config();
    debug("\t             New status: 0x%x \n",_conf.status);
  }
}

void VirtioNet::add_receive_buffer(){

  auto* pkt = bufstore().get_buffer();
  // get a pointer to a virtionet header
  auto* vnet = pkt + sizeof(Packet) - sizeof(virtio_net_hdr);

  debug2("<VirtioNet> Added receive-bufer @ 0x%x \n", (uint32_t)buf);

  Token token1 {
    {vnet, sizeof(virtio_net_hdr)},
      Token::IN };

  Token token2 {
    {vnet + sizeof(virtio_net_hdr), bufsize()},
      Token::IN };

  std::array<Token, 2> tokens {{ token1, token2 }};
  rx_q.enqueue(tokens);
}

std::unique_ptr<Packet>
VirtioNet::recv_packet(uint8_t* data, uint16_t size)
{
  auto* ptr = (Packet*) (data + sizeof(VirtioNet::virtio_net_hdr) - sizeof(Packet));
  new (ptr) Packet(bufsize(), size - sizeof(virtio_net_hdr), &bufstore());

  return std::unique_ptr<Packet> (ptr);
}

void VirtioNet::service_queues(){
  debug2("<RX Queue> %i new packets \n",
         rx_q.new_incoming());

  /** For RX, we dequeue, add new buffers and let receiver is responsible for
      memory management (they know when they're done with the packet.) */
  uint16_t dequeued_rx = 0;
  uint16_t dequeued_tx = 0;

  rx_q.disable_interrupts();
  tx_q.disable_interrupts();
  // A zipper, alternating between sending and receiving
  while (rx_q.new_incoming() or tx_q.new_incoming()) {

    // Do one RX-packet
    while (rx_q.new_incoming()) {
      auto res = rx_q.dequeue();
      auto pckt_ptr = recv_packet(res.data(), res.size());
      Link::receive(std::move(pckt_ptr));

      // Requeue a new buffer
      add_receive_buffer();

      dequeued_rx++;
    }

    // Do one TX-packet
    while (tx_q.new_incoming()) {
      // FIXME Unfortunately dequeue is not working here
      // I am guessing that Linux is eating the buffers
      tx_q.dequeue();

      // release the buffer back into bufferstore
      auto data = tx_ringq.front();
      tx_ringq.pop_front();
      bufstore().release(data);

      dequeued_tx++;
    }

  }
  rx_q.enable_interrupts();
  tx_q.enable_interrupts();

  // Let virtio know we have increased receive capacity
  if (dequeued_rx)
    rx_q.kick();

  debug2("<VirtioNet> Service loop about to kick RX if %i \n",
         dequeued_rx);
  // If we have a transmit queue, eat from it, otherwise let the stack know we
  // have increased transmit capacity
  if (dequeued_tx) {
    debug("<VirtioNet>%i dequeued, transmitting backlog\n", dequeued_tx);

    // transmit as much as possible from the buffer
    if (transmit_queue_){
      transmit(std::move(transmit_queue_));
    } else {
      debug("<VirtioNet> Transmit queue is empty \n");
    }

    // If we now emptied the buffer, offer packets to stack
    if (!transmit_queue_ && tx_q.num_free() > 1)
      transmit_queue_available_event_(tx_q.num_free() / 2);
    else
      debug("<VirtioNet> No event: !transmit q %i, num_avail %i \n",
            !transmit_queue_, tx_q.num_free());
  }

  debug("<VirtioNet> Done servicing queues\n");
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
#include <cstdlib>
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
  while (tx_q.num_free() and tail) {
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

    if (!tail) break;
  }

  // Notify virtio about new packets
  if (LIKELY(transmitted)) {
    begin_deferred_kick();
  }

  // Buffer the rest
  if (UNLIKELY(tail)) {
    debug("Buffering remaining packets..\n");
    add_to_tx_buffer(std::move(tail));
  }
}

void VirtioNet::enqueue(net::Packet* pckt) {

  // This setup requires all tokens to be pre-chained like in SanOS
  Token token1 {{(uint8_t*) &empty_header, sizeof(virtio_net_hdr)},
      Token::OUT };

  Token token2{ { pckt->buffer(), pckt->size() }, Token::OUT };

  std::array<Token, 2> tokens {{ token1, token2 }};

  // Enqueue scatterlist, 2 pieces readable, 0 writable.
  tx_q.enqueue(tokens);

  // have to release the packet data because virtio owns it now
  // but to do that the packet has to be unique here
  tx_ringq.push_back((uint8_t*) pckt);
}

void VirtioNet::begin_deferred_kick()
{
#ifdef NO_DEFERRED_KICK
  tx_q.kick();
#else
  if (!deferred_kick) {
    deferred_kick = true;
    deferred_devices.push_back(this);
    IRQ_manager::get().register_irq(deferred_intr);
  }
#endif
}
void VirtioNet::handle_deferred_devices()
{
#ifndef NO_DEFERRED_KICK
  for (auto* dev : deferred_devices) {
    // kick transmitq
    dev->deferred_kick = false;
    dev->tx_q.kick();
  }
  deferred_devices.clear();
#endif
}

void VirtioNet::deactivate()
{
  /// disable interrupts on virtio queues
  rx_q.disable_interrupts();
  tx_q.disable_interrupts();
  ctrl_q.disable_interrupts();

  /// mask off MSI-X vectors
  if (is_msix())
      deactivate_msix();
}

#include <kernel/pci_manager.hpp>

/** Register VirtioNet's driver factory at the PCI_manager */
struct Autoreg_virtionet {
  Autoreg_virtionet() {
    PCI_manager::register_driver<hw::Nic>(hw::PCI_Device::VENDOR_VIRTIO, 0x1000, &VirtioNet::new_instance);
  }
} autoreg_virtionet;
