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
#include <vector>
struct vmxnet3_dma;
struct vmxnet3_rx_desc;
struct vmxnet3_rx_comp;

class vmxnet3 : public net::Link_layer<net::Ethernet>
{
public:
  using Link          = net::Link_layer<net::Ethernet>;
  using Link_protocol = Link::Protocol;
  static const int ETH_FRAME_LEN = 1514;
  static const int NUM_RX_QUEUES = 1;
  static const int NUM_TX_DESC   = 128;
  static const int NUM_RX_DESC   = 128;

  static std::unique_ptr<Nic> new_instance(hw::PCI_Device& d)
  { return std::make_unique<vmxnet3>(d); }

  /** Human readable name. */
  const char* driver_name() const override {
    return "vmxnet3";
  }

  /** Mac address. */
  const MAC::Addr& mac() const noexcept override
  {
    return this->hw_addr;
  }

  uint16_t MTU() const noexcept override {
    return 1500;
  }

  uint16_t packet_len() const noexcept
  {        // ethernet + vlan + fcs
    return ETH_FRAME_LEN + 4 + 4;
  }

  net::downstream create_physical_downstream()
  { return {this, &vmxnet3::transmit}; }

  net::Packet_ptr create_packet(int) override;

  /** Linklayer input. Hooks into IP-stack bottom, w.DOWNSTREAM data.*/
  void transmit(net::Packet_ptr pckt);

  /** Constructor. @param pcidev an initialized PCI device. */
  vmxnet3(hw::PCI_Device& pcidev);

  /** Space available in the transmit queue, in packets */
  size_t transmit_queue_available() override {
    return tx_tokens_free();
  }

  void deactivate() override;

  void move_to_this_cpu() override;

private:
  void msix_evt_handler();
  void msix_xmit_handler();
  void msix_recv_handler();
  void enable_intr(uint8_t idx) noexcept;
  void disable_intr(uint8_t idx) noexcept;

  int  tx_tokens_free() const noexcept;
  bool can_transmit() const noexcept;
  void transmit_data(uint8_t* data, uint16_t);
  net::Packet_ptr recv_packet(uint8_t* data, uint16_t);

  // tx/rx ring state
  struct ring_stuff {
    uint8_t* buffers[NUM_TX_DESC];
    int producers  = 0;
    int prod_count = 0;
    int consumers  = 0;
  };
  struct rxring_state {
    uint8_t* buffers[NUM_RX_DESC];
    vmxnet3_rx_desc* desc0 = nullptr;
    vmxnet3_rx_desc* desc1 = nullptr;
    vmxnet3_rx_comp* comp  = nullptr;
    int index = 0;
    int producers  = 0;
    int prod_count = 0;
    int consumers  = 0;
  };
  void refill(rxring_state&);

  bool     check_version();
  uint16_t check_link();
  bool     reset();
  uint32_t command(uint32_t cmd);
  void     retrieve_hwaddr();
  void     set_hwaddr(MAC::Addr&);

  hw::PCI_Device& pcidev;
  std::vector<uint8_t> irqs;
  uintptr_t       iobase;
  uintptr_t       ptbase;
  MAC::Addr    hw_addr;
  vmxnet3_dma*    dma;

  ring_stuff tx;
  rxring_state rx[NUM_RX_QUEUES];
  // deferred transmit dma
  int      transmit_idx = -1;
  uint8_t  deferred_irq  = 0;
  bool     deferred_kick = false;
  static void handle_deferred();
  // sendq as packet chain
  net::Packet_ptr sendq = nullptr;
};
