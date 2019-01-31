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

#ifndef VIRTIO_SOLO5NET_HPP
#define VIRTIO_SOLO5NET_HPP

#include <common>
#include <hw/pci_device.hpp>
#include <net/buffer_store.hpp>
#include <net/link_layer.hpp>
#include <net/ethernet/ethernet.hpp>
#include <delegate>
#include <deque>
#include <statman>

extern "C" {
#include <solo5/solo5.h>
}

class Solo5Net : public net::Link_layer<net::Ethernet> {
public:
  using Link          = net::Link_layer<net::Ethernet>;
  using Link_protocol = Link::Protocol;

  static std::unique_ptr<Nic> new_instance()
  {
    return std::make_unique<Solo5Net>();
  }

  /** Human readable name. */
  const char* driver_name() const override;

  /** Mac address. */
  const MAC::Addr& mac() const noexcept override {
    return mac_addr;
  }

  uint16_t MTU() const noexcept override
  { return 1500; }

  uint16_t packet_len() const noexcept {
    return sizeof(net::ethernet::Header) + MTU();
  }

  net::downstream create_physical_downstream() override
  { return {this, &Solo5Net::transmit}; }

  /** Linklayer input. Hooks into IP-stack bottom, w.DOWNSTREAM data.*/
  void transmit(net::Packet_ptr pckt);

  /** Constructor. @param pcidev an initialized PCI device. */
  Solo5Net();

  /** Space available in the transmit queue, in packets */
  size_t transmit_queue_available() override {
    return 1000; // any big random number for now
  }

  net::Packet_ptr create_packet(int) override;

  auto& bufstore() noexcept { return bufstore_; }

  void move_to_this_cpu() override {};

  void deactivate() override;

  void flush() override {};

  void poll() override;

private:
  MAC::Addr mac_addr;
  std::unique_ptr<net::Packet> recv_packet();
  /** Stats */
  uint64_t& packets_rx_;
  uint64_t& packets_tx_;

  net::BufferStore bufstore_;

};

#endif
