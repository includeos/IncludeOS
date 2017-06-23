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

#include "solo5net.hpp"
#include <net/packet.hpp>
#include <kernel/irq_manager.hpp>
#include <kernel/syscalls.hpp>
#include <hw/pci.hpp>
#include <stdio.h>
#include <malloc.h>
#include <string.h>

extern "C" {
#include <solo5.h>
}

using namespace net;

constexpr size_t MTU_ = 1520;
constexpr size_t BUFFER_CNT = 1000;

// XXX: do we really need a bufstore?
BufferStore solo5_bufstore{ BUFFER_CNT,  MTU_ };

const char* Solo5Net::driver_name() const { return "Solo5Net"; }

Solo5Net::Solo5Net(hw::PCI_Device& d)
  : Link(Link_protocol{{this, &Solo5Net::transmit}, mac()},
         2048u, sizeof(net::Packet) + MTU()),
    packets_rx_{Statman::get().create(Stat::UINT64, device_name() + ".packets_rx").get_uint64()},
    packets_tx_{Statman::get().create(Stat::UINT64, device_name() + ".packets_tx").get_uint64()}
{
  INFO("Solo5Net", "Driver initializing");
}

#include <cstdlib>
void Solo5Net::transmit(net::Packet_ptr pckt) {
  net::Packet_ptr tail = std::move(pckt);

  // Transmit all we can directly
  while (tail) {
    // next in line
    auto next = tail->detach_tail();
    // write data to network
    // explicitly release the data to prevent destructor being called
    net::Packet* pckt = tail.release();
    uint8_t *buf = pckt->buf();

    printf("Solo5 writing packet %i bytes \n", pckt->size());
    solo5_net_write_sync(buf, pckt->size());

    tail = std::move(next);
    // Stat increase packets transmitted
    packets_tx_++;
  }

  // Buffer the rest
  if (UNLIKELY(tail)) {
    INFO("solo5net", "Could not send all packets..\n");
  }
}

std::unique_ptr<Packet>
Solo5Net::create_packet(int link_offset)
{
  auto buffer = solo5_bufstore.get_buffer();
  auto* pckt = (net::Packet*) buffer.addr;

  new (pckt) net::Packet(link_offset, 0, MTU_, &solo5_bufstore);
  return net::Packet_ptr(pckt);
}

std::unique_ptr<Packet>
Solo5Net::recv_packet()
{
  auto buffer = solo5_bufstore.get_buffer();
  auto* pckt = (net::Packet*) buffer.addr;
  int size = MTU_;
  new (pckt) net::Packet(0, size, MTU_, &solo5_bufstore);
  uint8_t *buf = pckt->buf();
  memset(buf, 0, size);
  // Populate the packet buffer with new packet, if any
  if (solo5_net_read_sync(buf, &size) == 0) {
    // Adjust packet size to match received data
    //printf("Solo5 data size: %i \n", size);
    if (size) {
      pckt->set_data_end(size);
      //printf("Solo5 Packet size: %i \n", pckt->size());
      return net::Packet_ptr(pckt);
    }
  }
  printf("Solo5 didn't get data. Size: %i \n", size);
  return nullptr;
}

void Solo5Net::poll()
{
  auto pckt_ptr = recv_packet();
  if (pckt_ptr != nullptr)
    Link::receive(std::move(pckt_ptr));
}

void Solo5Net::deactivate()
{
  INFO("Solo5Net", "deactivate");
}

#include <kernel/solo5_manager.hpp>

struct Autoreg_solo5net {
  Autoreg_solo5net() {
    Solo5_manager::register_driver<hw::Nic>(PCI::VENDOR_SOLO5, 0x1000,
                                            &Solo5Net::new_instance);
  }
} autoreg_solo5net;
