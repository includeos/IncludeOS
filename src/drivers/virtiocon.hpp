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

#pragma once
#ifndef VIRTIO_CONSOLE_HPP
#define VIRTIO_CONSOLE_HPP

#include <common>
#include <delegate>
#include <hw/pci_device.hpp>
#include <virtio/virtio.hpp>

/**
 * http://docs.oasis-open.org/virtio/virtio/v1.0/csprd05/virtio-v1.0-csprd05.html#x1-2180003
 *
 * The virtio console device is a simple device for data input
 * and output. A device MAY have one or more ports. Each port has
 * a pair of input and output virtqueues. Moreover, a device has a
 * pair of control IO virtqueues. The control virtqueues are used
 * to communicate information between the device and the driver
 * about ports being opened and closed on either side of the
 * connection, indication from the device about whether a
 * particular port is a console port, adding new ports, port
 * hot-plug/unplug, etc., and indication from the driver about
 * whether a port or a device was successfully added,
 * port open/close, etc. For data IO, one or more empty buffers are
 * placed in the receive queue for incoming data and outgoing
 * characters are placed in the transmit queue.
 *
 **/

class VirtioCon : public Virtio
{
public:
  std::string device_name() const {
    return "virtiocon" + std::to_string(m_index);
  }

  // returns the sizes of this console
  size_t rows() const noexcept
  {
    return config.rows;
  }
  size_t cols() const noexcept
  {
    return config.cols;
  }

  void write (const void* data, size_t len);

  /** Constructor. @param pcidev an initialized PCI device. */
  VirtioCon(hw::PCI_Device& pcidev);

private:
  struct console_config
  {
    uint16_t cols;
    uint16_t rows;
    uint32_t max_nr_ports;
    uint32_t emerg_wr;
  };

  struct console_control
  {
    uint32_t id;   // port number, but lets call it id so we need a comment to explain what it is :)
    uint16_t event;
    uint16_t value;
  };

  struct console_resize
  {
    uint16_t cols;
    uint16_t rows;
  };

  /** Get virtio PCI config. @see Virtio::get_config.*/
  void get_config();

  void event_handler();
  void msix_recv_handler();
  void msix_xmit_handler();

  Virtio::Queue rx;     // 0
  Virtio::Queue tx;     // 1
  Virtio::Queue ctl_rx; // 2
  Virtio::Queue ctl_tx; // 3

  // configuration as read from paravirtual PCI device
  console_config config;
  const int m_index = 0;
};

#endif
