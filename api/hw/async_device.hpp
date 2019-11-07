
#pragma once
#include <kernel/events.hpp>
#include <hw/usernet.hpp>
#include <deque>

namespace hw
{
template <typename Driver>
class Async_device {
public:
  using transmit_func = delegate<void(net::Packet_ptr)>;

  void connect(Async_device& other) {
    this->set_transmit({&other, &Async_device::receive});
  }

  Async_device (Driver& nic)
      : m_nic(nic)
  {
    this->event_id = Events::get().subscribe(
      [this] {
        while(! queue.empty()) {
          this->driver_receive(std::move(queue.front()));
          queue.pop_front();
        }
        this->m_nic.signal_tqa();
      });
  }

  void receive(net::Packet_ptr pckt) {
    queue.push_back(std::move(pckt));
    Events::get().trigger_event(event_id);
  }

  void set_transmit(transmit_func func) {
    this->m_nic.set_transmit_forward(std::move(func));
  }

  auto& nic() {
    return this->m_nic;
  }

private:
  inline void driver_receive(net::Packet_ptr packet) {
    this->m_nic.receive(std::move(packet));
  }
  Driver& m_nic;
  int event_id = 0;
  std::deque<net::Packet_ptr> queue;
};
} // hw
