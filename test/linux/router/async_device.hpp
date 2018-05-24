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
#include <kernel/events.hpp>
#include <drivers/usernet.hpp>
#include <net/inet>
#include <deque>

class Async_device {
public:
  using transmit_func = delegate<void(net::Packet_ptr)>;
  using receive_func = delegate<void(net::Packet_ptr)>;

  void connect(Async_device& other) {
    this->set_transmit({&other, &Async_device::receive});
  }

  Async_device (const uint16_t MTU)
  {
    this->driver = &UserNet::create(MTU);
    this->event_id = Events::get().subscribe(
      [this] {
        while(! queue.empty()) {
          this->driver_receive(std::move(queue.front()));
          queue.pop_front();
        }
      });
  }

  void receive(net::Packet_ptr pckt) {
    queue.push_back(std::move(pckt));
    Events::get().trigger_event(event_id);
  }

  void set_transmit(transmit_func func) {
    assert(driver != nullptr);
    driver->set_transmit_forward(std::move(func));
  }

  auto* get_driver() {
    return this->driver;
  }

private:
  inline void driver_receive(net::Packet_ptr packet) {
    this->driver->receive(std::move(packet));
  }
  UserNet* driver = nullptr;
  int event_id = 0;
  std::deque<net::Packet_ptr> queue;
};
