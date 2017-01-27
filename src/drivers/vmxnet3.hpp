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

#include <hw/pci_device.hpp>
#include <net/link_layer.hpp>
#include <net/ethernet/ethernet.hpp>
#include <deque>
#include <vector>

#define ETH_FRAME_LEN       1514
#define VMXNET3_NUM_TX_DESC  128
#define VMXNET3_NUM_RX_DESC  128
struct vmxnet3_dma;

class vmxnet3 : public net::Link_layer<net::Ethernet>
{
public:
  using Link          = net::Link_layer<net::Ethernet>;
  using Link_protocol = Link::Protocol;
  static const int NUM_RX_QUEUES = 1;

  static std::unique_ptr<Nic> new_instance(hw::PCI_Device& d)
  { return std::make_unique<vmxnet3>(d); }

  /** Human readable name. */
  const char* driver_name() const override {
    return "vmxnet3";
  }

  /** Mac address. */
  const hw::MAC_addr& mac() const noexcept override
  {
    return this->hw_addr;
  }

  uint16_t MTU() const noexcept override
  {        // ethernet + vlan + fcs
    return ETH_FRAME_LEN + 4 + 4;
  }

  net::downstream create_physical_downstream()
  { return {this, &vmxnet3::transmit}; }

  /** Linklayer input. Hooks into IP-stack bottom, w.DOWNSTREAM data.*/
  void transmit(net::Packet_ptr pckt);

  /** Constructor. @param pcidev an initialized PCI device. */
  vmxnet3(hw::PCI_Device& pcidev);

  /** Space available in the transmit queue, in packets */
  size_t transmit_queue_available() override {
    return bufstore().available();
  }

  /** Number of incoming packets waiting in the RX-queue */
  size_t receive_queue_waiting() override {
    return 0;
  }

  void deactivate() override;

private:
  void msix_evt_handler();
  void msix_xmit_handler();
  void msix_recv_handler();
  void unused_handler();
  void refill_rx(int q);
  void enable_intr(uint8_t idx) noexcept;
  void disable_intr(uint8_t idx) noexcept;

  bool can_transmit() const noexcept;

  net::Packet_ptr recv_packet(uint8_t* data, uint16_t);

  bool     check_version();
  uint16_t check_link();
  void     reset();
  uint32_t command(uint32_t cmd);
  void     retrieve_hwaddr();
  void     set_hwaddr(hw::MAC_addr&);

  uintptr_t       iobase;
  uintptr_t       ptbase;
  hw::MAC_addr    hw_addr;
  vmxnet3_dma*    dma;
  std::vector<uint8_t> irqs;

  // ring counters
  struct ring_stuff {
    int producers  = 0;
    int prod_count = 0;
    int consumers  = 0;
  };
  ring_stuff tx;
  ring_stuff rx[NUM_RX_QUEUES];
  uint8_t* rxq_buffers[VMXNET3_NUM_RX_DESC];
  uint8_t* txq_buffers[VMXNET3_NUM_TX_DESC];

  std::deque<net::Packet_ptr> sendq;
};
