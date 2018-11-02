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
#include <net/ethernet/ethernet_8021q.hpp>
#include <deque>
#include <vector>

class e1000 : public net::Link_layer<net::Ethernet>
{
public:
  using Link          = net::Link_layer<net::Ethernet>;
  using Link_protocol = Link::Protocol;
  static const int DRIVER_OFFSET = 2;
  static const int NUM_TX_DESC   = 64;
  static const int NUM_TX_QUEUE  = 64;
  static const int NUM_RX_DESC   = 64;

  static std::unique_ptr<Nic> new_instance(hw::PCI_Device& d, const uint16_t MTU)
  { return std::make_unique<e1000>(d, MTU); }

  const char* driver_name() const override {
    return "e1000";
  }

  const MAC::Addr& mac() const noexcept override {
    return this->hw_addr;
  }

  uint16_t MTU() const noexcept override {
    return m_mtu;
  }

  uint16_t packet_len() const noexcept {
    return frame_offset_link() + MTU();
  }

  net::downstream create_physical_downstream() override
  { return {this, &e1000::transmit}; }

  net::Packet_ptr create_packet(int) override;

  /** Linklayer input. Hooks into IP-stack bottom, w.DOWNSTREAM data.*/
  void transmit(net::Packet_ptr pckt);

  /** Constructor. @param pcidev an initialized PCI device. */
  e1000(hw::PCI_Device& pcidev, uint16_t MTU);

  /** Space available in the transmit queue, in packets */
  size_t transmit_queue_available() override {
    if (sendq_size >= NUM_TX_QUEUE) return 0;
    return free_transmit_descr() + NUM_TX_QUEUE - sendq_size;
  }

  auto& bufstore() noexcept { return bufstore_; }

  void flush() override;

  void deactivate() override;

  void move_to_this_cpu() override;

  void poll() override;

private:
  void intr_enable();
  void intr_disable();
  void intr_cause_clear();
  void link_up();
  void retrieve_hw_addr();
  void config_msix();

  void wait_millis(int);

  void init_filters();
  void set_filter(int, MAC::Addr);
  uint64_t construct_filter(MAC::Addr);

  void     detect_eeprom();
  uint32_t read_eeprom(uint8_t addr);

  uint32_t read_cmd(uint16_t cmd);
  void     write_cmd(uint16_t cmd, uint32_t val);

  net::Packet_ptr recv_packet(uint8_t*, uint16_t);
  uintptr_t       new_rx_packet();
  void event_handler();
  void receive_handler();
  void transmit_handler();
  uint16_t free_transmit_descr() const noexcept;
  bool can_transmit() const noexcept;
  void transmit_data(uint8_t*, uint16_t);
  void do_release_transmitted();
  void xmit_kick();
  static void do_deferred_xmit();

  hw::PCI_Device& m_pcidev;
  std::vector<uint8_t> irqs;
  bool         use_mmio = false;
  bool         use_eeprom = false;
  bool         use_msix = false;
  bool         link_state_up = false;
  uint16_t     io_base;
  uintptr_t    shm_base;
  MAC::Addr      hw_addr;
  const uint16_t m_mtu;

  struct rx_desc
  {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t  status;
    uint8_t  errors;
    uint16_t vlan_tag;
  } __attribute__((packed, aligned(16)));
  struct tx_desc
  {
    uint64_t addr;
    uint16_t length;
    uint8_t  cso;
    uint8_t  cmd;
    uint8_t  status;
    uint8_t  css;
    uint16_t vlan_tag;
  } __attribute__((packed, aligned(16)));

  struct rx_t {
    rx_desc desc[NUM_RX_DESC];
    uint16_t current = 0;
  } rx;

  struct tx_t {
    tx_desc desc[NUM_TX_DESC];
    std::deque<net::Packet*> sent;
    uint16_t current = 0;
    uint16_t sent_id = 0;
    bool deferred = false;
  } tx;

  // sendq as packet chain
  net::Packet_ptr  sendq = nullptr;
  size_t           sendq_size = 0;
  net::BufferStore bufstore_;
};
