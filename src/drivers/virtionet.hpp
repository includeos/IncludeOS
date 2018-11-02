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

/**
   @note This virtionet implementation was very much inspired by
   SanOS, (C) Michael Ringgaard. All due respect.

   STANDARD:

   We're aiming for standard compliance:

   Virtio 1.0, OASIS Committee Specification Draft 01
   (http://docs.oasis-open.org/virtio/virtio/v1.0/csd01/virtio-v1.0-csd01.pdf)

   In the following abbreviated to Virtio 1.01

   ...Alas, nobody's using it yet, so we're stuck with "legacy" for now.
*/
#ifndef VIRTIO_VIRTIONET_HPP
#define VIRTIO_VIRTIONET_HPP

#include <common>
#include <hw/pci_device.hpp>
#include <virtio/virtio.hpp>
#include <net/packet.hpp>
#include <net/buffer_store.hpp>
#include <net/link_layer.hpp>
#include <net/ethernet/ethernet.hpp>
#include <net/ethernet/ethernet_8021q.hpp> // vlan header size
#include <delegate>
#include <deque>
#include <statman>

/** Virtio Net Features. From Virtio Std. 5.1.3 */

/* Device handles packets with partial checksum. This “checksum offload”
   is a common feature on modern network cards.*/
#define VIRTIO_NET_F_CSUM 0

/* Driver handles packets with partial checksum. */
#define VIRTIO_NET_F_GUEST_CSUM 1

/* Control channel offloads reconfiguration support. */
#define VIRTIO_NET_F_CTRL_GUEST_OFFLOADS 2

/* Device has given MAC address. */
#define VIRTIO_NET_F_MAC 5

/* Driver can receive TSOv4. */
#define VIRTIO_NET_F_GUEST_TSO4 7

/* Driver can receive TSOv6. */
#define VIRTIO_NET_F_GUEST_TSO6 8

/* Driver can receive TSO with ECN.*/
#define VIRTIO_NET_F_GUEST_ECN 9

/* Driver can receive UFO. (UFO?? WTF!) */
#define VIRTIO_NET_F_GUEST_UFO 10

/* Device can receive TSOv4. */
#define VIRTIO_NET_F_HOST_TSO4 11

/* Device can receive TSOv6. */
#define VIRTIO_NET_F_HOST_TSO6 12

/* Device can receive TSO with ECN. */
#define VIRTIO_NET_F_HOST_ECN 13

/* Device can receive UFO.*/
#define VIRTIO_NET_F_HOST_UFO 14

/* Driver can merge receive buffers. */
#define VIRTIO_NET_F_MRG_RXBUF 15

/* Configuration status field is available. */
#define VIRTIO_NET_F_STATUS 16

/* Control channel is available.*/
#define VIRTIO_NET_F_CTRL_VQ 17

/* Control channel RX mode support.*/
#define VIRTIO_NET_F_CTRL_RX 18

/* Control channel VLAN filtering.*/
#define VIRTIO_NET_F_CTRL_VLAN 19

/* Driver can send gratuitous packets.*/
#define VIRTIO_NET_F_GUEST_ANNOUNCE 21

/* Device supports multiqueue with automatic receive steering.*/
#define VIRTIO_NET_F_MQ 22

/* Set MAC address through control channel.*/
#define VIRTIO_NET_F_CTRL_MAC_ADDR 23


// From Virtio 1.01, 5.1.4
#define VIRTIO_NET_S_LINK_UP  1
#define VIRTIO_NET_S_ANNOUNCE 2

/** Virtio-net device driver.  */
class VirtioNet : Virtio, public net::Link_layer<net::Ethernet> {
public:
  using Link          = net::Link_layer<net::Ethernet>;
  using Link_protocol = Link::Protocol;

  static std::unique_ptr<Nic> new_instance(hw::PCI_Device& d, const uint16_t MTU)
  { return std::make_unique<VirtioNet>(d, MTU); }

  /** Human readable name. */
  const char* driver_name() const override {
    return "VirtioNet";
  }

  /** Mac address. */
  const MAC::Addr& mac() const noexcept override
  { return _conf.mac; }

  uint16_t MTU() const noexcept override
  { return 1500; }

  uint16_t max_packet_len() const noexcept {
    return sizeof(net::ethernet::VLAN_header) + MTU();
  }

  net::Packet_ptr create_packet(int) override;

  net::downstream create_physical_downstream() override
  { return {this, &VirtioNet::transmit}; }


  /** Linklayer input. Hooks into IP-stack bottom, w.DOWNSTREAM data.*/
  void transmit(net::Packet_ptr pckt);

  /** Constructor. @param pcidev an initialized PCI device. */
  VirtioNet(hw::PCI_Device& pcidev, uint16_t MTU);

  /** Space available in the transmit queue, in packets */
  size_t transmit_queue_available() override {
    return tx_q.num_free() / 2;
  }

  bool link_up() const noexcept;

  auto& bufstore() noexcept { return bufstore_; }

  void deactivate() override;

  void flush() override {
    tx_q.kick();
  };

  void move_to_this_cpu() override;

  void poll() override;

private:
  hw::PCI_Device& m_pcidev;

  struct virtio_net_hdr {
    uint8_t flags;
    uint8_t gso_type;
    uint16_t hdr_len;          // Ethernet + IP + TCP/UDP headers
    uint16_t gso_size;         // Bytes to append to hdr_len per frame
    uint16_t csum_start;       // Position to start checksumming from
    uint16_t csum_offset;      // Offset after that to place checksum
  }__attribute__((packed));

  /** Virtio std. § 5.1.6.1:
      "The legacy driver only presented num_buffers in the struct virtio_net_hdr when VIRTIO_NET_F_MRG_RXBUF was not negotiated; without that feature the structure was 2 bytes shorter." */
  struct virtio_net_hdr_nomerge {
    uint8_t flags;
    uint8_t gso_type;
    uint16_t hdr_len;          // Ethernet + IP + TCP/UDP headers
    uint16_t gso_size;         // Bytes to append to hdr_len per frame
    uint16_t csum_start;       // Position to start checksumming from
    uint16_t csum_offset;      // Offset after that to place checksum
    uint16_t num_buffers;
  }__attribute__((packed));

  Virtio::Queue rx_q;
  Virtio::Queue tx_q;
  Virtio::Queue ctrl_q;

  // From Virtio 1.01, 5.1.4
  struct config{
    MAC::Addr mac;
    uint16_t status;

    //Only valid if VIRTIO_NET_F_MQ
    uint16_t max_virtq_pairs = 0;
  }_conf;

  //sizeof(config) if VIRTIO_NET_F_MQ, else sizeof(config) - sizeof(uint16_t)
  int _config_length = sizeof(config);

  /** Get virtio PCI config. @see Virtio::get_config.*/
  void get_config();

  /** Add packet to transmit ring */
  void enqueue_tx(net::Packet* pckt);

  /** Handle device IRQ.
      Will look for config changes and service RX/TX queues as necessary.*/
  void msix_recv_handler();
  void msix_xmit_handler();
  void msix_conf_handler();

  /** Legacy IRQ handler */
  void legacy_handler();

  /** Allocate and queue buffer from bufstore_ in RX queue. */
  void add_receive_buffer(uint8_t*);

  std::unique_ptr<net::Packet> recv_packet(uint8_t* data, uint16_t sz);

  void begin_deferred_kick();
  bool deferred_kick = false;
  static void handle_deferred_devices();

  net::BufferStore bufstore_;

  /** Stats */
  uint64_t& stat_sendq_max_;
  uint64_t& stat_sendq_now_;
  uint64_t& stat_sendq_limit_dropped_;
  uint64_t& stat_rx_refill_dropped_;
  uint64_t& stat_bytes_rx_total_;
  uint64_t& stat_bytes_tx_total_;
  uint64_t& stat_packets_rx_total_;
  uint64_t& stat_packets_tx_total_;

  std::deque<net::Packet_ptr> sendq{};

};

#endif
