/**
 * User-space network card
 * by gonzo
**/
#pragma once

#include <net/buffer_store.hpp>
#include <net/link_layer.hpp>
#include <net/ethernet/ethernet.hpp>
#include <delegate>

class UserNet : public net::Link_layer<net::Ethernet> {
public:
  using Link          = net::Link_layer<net::Ethernet>;
  using Link_protocol = Link::Protocol;
  static constexpr MAC::Addr MAC_ADDRESS = {1, 2, 3, 4, 5, 6};

  const char* driver_name() const override {
    return "UserNet";
  }

  const MAC::Addr& mac() const noexcept override
  { return MAC_ADDRESS; }

  uint16_t MTU() const noexcept override
  { return 1500; }

  uint16_t packet_len() const noexcept {
    return Link::Protocol::header_size() + MTU();
  }

  net::Packet_ptr create_packet(int) override;

  size_t frame_offset_device() override
  { return sizeof(driver_hdr); };

  net::downstream create_physical_downstream()
  { return {this, &UserNet::transmit}; }

  /** the function called from transmit() **/
  typedef delegate<void(net::Packet_ptr)> forward_t;
  void set_transmit_forward(forward_t func) {
    this->transmit_forward_func = func;
  }

  /** packets going out to network **/
  void transmit(net::Packet_ptr);

  /** packets coming in from network **/
  void feed(void*, net::BufferStore* = nullptr);
  void feed(net::Packet_ptr);
  void write(const void* data, int len);

  /** Space available in the transmit queue, in packets */
  size_t transmit_queue_available() override;

  void deactivate() override {}
  void move_to_this_cpu() override {}
  void flush() override {}
  void poll() override {}

  UserNet();

  struct driver_hdr {
    uint32_t len;
    uint16_t padding;
  }__attribute__((packed));

private:
  forward_t transmit_forward_func;
};
