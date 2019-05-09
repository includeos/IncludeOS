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

#ifndef HW_NIC_HPP
#define HW_NIC_HPP

#include "mac_addr.hpp"
#include <net/inet_common.hpp>
#include "device.hpp"

#define NIC_SENDQ_LIMIT_DEFAULT  4096
#define NIC_BUFFER_LIMIT_DEFAULT 4096

namespace hw {

  /**
   *  A public interface Network cards
   */
  class Nic : public Device {
  public:
    using upstream    = delegate<void(net::Packet_ptr)>;
    using downstream  = net::downstream_link;

    /** Maximum number of packets allowed in dynamic send queue **/
    static constexpr uint32_t sendq_limit_default  = NIC_SENDQ_LIMIT_DEFAULT;

    /** Maximum number of packets that can be created during RX-ring refill **/
    static constexpr uint32_t buffer_limit_default = NIC_BUFFER_LIMIT_DEFAULT;

    enum class Proto {ETH, IEEE802111};

    virtual Proto proto() const = 0;

    /** Human-readable driver name */
    virtual const char* driver_name() const = 0;

    /** Human-readable interface/device name (eg. eth0) */
    virtual std::string device_name() const override = 0;

    Device::Type device_type() const noexcept override
    { return Device::Type::Nic; }

    /** The mac address. */
    virtual const MAC::Addr& mac() const noexcept = 0;

    virtual uint16_t MTU() const noexcept = 0;

    /** Downstream delegate factory */
    virtual downstream create_link_downstream() = 0;

    /** Protocol handler getters **/
    virtual net::upstream_ip& ip4_upstream() = 0;
    virtual net::upstream_ip& ip6_upstream() = 0;
    virtual upstream& arp_upstream() = 0;

    /** Protocol handler setters */
    virtual net::downstream create_physical_downstream() = 0;
    virtual void set_ip4_upstream(net::upstream_ip handler) = 0;
    virtual void set_ip6_upstream(net::upstream_ip handler) = 0;
    virtual void set_arp_upstream(upstream handler) = 0;
    virtual void set_vlan_upstream(upstream handler) = 0;

    /** Number of bytes in a frame needed by the link layer **/
    virtual size_t frame_offset_link() const noexcept = 0;

    /**
     * Create a packet with appropriate size for the underlying link
     * @param layer_begin : offset in octets from the link-layer header
     */
    virtual net::Packet_ptr create_packet(int layer_begin) = 0;

    /** Subscribe to event for when there is more room in the tx queue */
    virtual void on_transmit_queue_available(net::transmit_avail_delg del)
    { tqa_events_.push_back(del); }

    virtual size_t transmit_queue_available() = 0;

    virtual void deactivate() override = 0;

    /** Stats getters **/
    virtual uint64_t get_packets_rx() = 0;
    virtual uint64_t get_packets_tx() = 0;
    virtual uint64_t get_packets_dropped() = 0;

    /** Move this nic to current CPU **/
    virtual void move_to_this_cpu() = 0;

    /** Flush remaining packets if possible. **/
    virtual void flush() override = 0;

    virtual ~Nic() {}

    /** Check for completed rx and pass rx packets up the stack */
    virtual void poll() = 0;

    /** Overridable MTU detection function per-network **/
    static uint16_t MTU_detection_override(int idx, uint16_t default_MTU);

    /** Set new buffer limit, where 0 means infinite **/
    void set_buffer_limit(uint32_t new_limit) {
      this->m_buffer_limit = new_limit;
    }
    uint32_t buffer_limit() const noexcept { return m_buffer_limit; }

    /** Set new sendq limit, where 0 means infinite **/
    void set_sendq_limit(uint32_t new_limit) {
      this->m_sendq_limit = new_limit;
    }
    uint32_t sendq_limit() const noexcept { return m_sendq_limit; }

    virtual void add_vlan([[maybe_unused]] const int id){}

  protected:
    /**
     *  Constructor
     *
     *  Constructed by the actual Nic Driver
     */
    Nic()
    {
      static int id_counter = 0;
      N = id_counter++;
    }

    std::vector<net::transmit_avail_delg> tqa_events_;

    void transmit_queue_available_event(size_t packets)
    {
      // early on its possible someone tries to transmit without subscribers
      if (tqa_events_.empty()) return;

      // divide up fairly
      size_t div = packets / tqa_events_.size();

      // give each handler a chance to take
      for (auto& del : tqa_events_)
        del(div);

      // hand out remaining
      for (auto& del : tqa_events_) {
        div = transmit_queue_available();
        if (!div) break;
        // give as much as possible
        del(div);
      }
    }

    bool buffers_still_available(uint32_t size) const noexcept {
      return this->buffer_limit() == 0 || size < this->buffer_limit();
    }
    bool sendq_still_available(uint32_t size) const noexcept {
      return this->sendq_limit() == 0 || size < this->sendq_limit();
    }

  private:
    int N;
    uint32_t m_buffer_limit = buffer_limit_default;
    uint32_t m_sendq_limit = sendq_limit_default;
    friend class Devices;
  };

} //< namespace hw

#endif // NIC_HPP
