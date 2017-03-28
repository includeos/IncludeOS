// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

#ifndef NET_IP4_ICMPv4_HPP
#define NET_IP4_ICMPv4_HPP

#include "packet_icmp4.hpp"
#include <map>
#include <timers>

namespace net {

  /**
   *  User friendly ICMP packet used in callback (icmp_func)
   */
  struct ICMP_packet {
    using Span = gsl::span<uint8_t>;

    uint16_t      id_{0};
    uint16_t      seq_{0};
    IP4::addr     src_{0,0,0,0};
    IP4::addr     dst_{0,0,0,0};
    icmp4::Type   type_{icmp4::Type::NO_REPLY};
    uint8_t       code_{0};
    uint16_t      checksum_{0};
    Span          payload_{nullptr, 0};

  public:
    ICMP_packet() {}

    ICMP_packet(uint16_t id, uint16_t seq, IP4::addr src, IP4::addr dst, icmp4::Type type, uint8_t code,
      uint16_t checksum, const Span& payload)
    : id_{id}, seq_{seq}, src_{src}, dst_{dst}, type_{type}, code_{code},
      checksum_{checksum}, payload_{payload}
    {}

    uint16_t id() const noexcept
    { return id_; }

    uint16_t seq() const noexcept
    { return seq_; }

    IP4::addr src() const noexcept
    { return src_; }

    IP4::addr dst() const noexcept
    { return dst_; }

    icmp4::Type type() const noexcept
    { return type_; }

    uint8_t code() const noexcept
    { return code_; }

    uint16_t checksum() const noexcept
    { return checksum_; }

    Span payload() const noexcept
    { return payload_; }

    operator bool() const noexcept
    { return type_ != icmp4::Type::NO_REPLY; }

    std::string to_string();
  }; // < struct ICMP_packet


  struct ICMPv4 {

    using Stack = IP4::Stack;
    using Tuple = std::pair<uint16_t, uint16_t>;  // identifier and sequence number
    using icmp_func = delegate<void(ICMP_packet)>;

    // Initialize
    ICMPv4(Stack&);

    // Input from network layer
    void receive(Packet_ptr);

    // Delegate output to network layer
    inline void set_network_out(downstream s)
    { network_layer_out_ = s; };

    /**
     *  Destination Unreachable sent from host because of port (UDP) or protocol (IP4) unreachable
     */
    void destination_unreachable(Packet_ptr pckt, icmp4::code::Dest_unreachable code);

    /**
     *
     */
    void redirect(icmp4::Packet& req, icmp4::code::Redirect code);

    /**
     *  Sending a Time Exceeded message from a host when fragment reassembly time exceeded (code 1)
     *  Sending a Time Exceeded message from a gateway when time to live exceeded in transit (code 0)
     */
    void time_exceeded(Packet_ptr pckt, icmp4::code::Time_exceeded code);

    /**
     *  Sending a Parameter Problem message if the gateway or host processing a datagram finds a problem with
     *  the header parameters such that it cannot complete processing the datagram. The message is only sent if
     *  the error caused the datagram to be discarded
     *  Code 0 means Pointer (uint8_t after checksum) indicates the error/identifies the octet where an error was detected
     *  in the IP header
     *  Code 1 means that a required option is missing
     */
    void parameter_problem(Packet_ptr pckt, uint8_t error);

    // May
    void timestamp_request(IP4::addr ip);
    void timestamp_reply(icmp4::Packet& req);

    void ping(IP4::addr ip);
    void ping(IP4::addr ip, icmp_func callback);

  private:
    static int request_id_; // message identifier for messages originating from IncludeOS
    Stack& inet_;
    downstream network_layer_out_ =   nullptr;
    uint8_t includeos_payload_[48] =  {'I','N','C','L','U','D',
                                      'E','O','S',1,2,3,4,5,
                                      'A','B','C','D','E','F','G','H',
                                      'I','J','K','L','M','N','O','P',
                                      'Q','R','S','T','U','V','W','X',
                                      'Y','Z',1,2,3,4,5,6,
                                      7,8};

    struct ICMP_callback {
      using icmp_func = ICMPv4::icmp_func;
      using Tuple = ICMPv4::Tuple;

      Tuple tuple;
      icmp_func callback;
      Timers::id_t timer_id;

      ICMP_callback(ICMPv4& icmp, Tuple t, icmp_func cb)
      : tuple{t}, callback{cb}
      {
        timer_id = Timers::oneshot(std::chrono::seconds(40), [&icmp, t](Timers::id_t) {
          icmp.remove_ping_callback(t);
        });
      }
    };  // < struct ICMP_callback

    std::map<Tuple, ICMP_callback> ping_callbacks_;

    void forward_to_transport_layer(icmp4::Packet& req);

    /**
     *  Find the ping-callback that this packet is a response to, execute it and erase the object
     *  from the ping_callbacks_ map
     */
    inline void execute_ping_callback(icmp4::Packet& ping_response) {
      // Find callback matching the reply
      auto it = ping_callbacks_.find(std::make_pair(ping_response.id(), ping_response.sequence()));

      if (it != ping_callbacks_.end()) {
        it->second.callback(ICMP_packet{ping_response.id(), ping_response.sequence(), ping_response.ip().ip_src(),
          ping_response.ip().ip_dst(), ping_response.type(), ping_response.code(), ping_response.checksum(), ping_response.payload()});
        Timers::stop(it->second.timer_id);
        ping_callbacks_.erase(it);
      }
    }

    /** Remove ICMP_callback from ping_callbacks_ map when its timer timeouts */
    inline void remove_ping_callback(Tuple key) {
      auto it = ping_callbacks_.find(key);

      if (it != ping_callbacks_.end()) {
        // Data back to user if no response found
        it->second.callback(ICMP_packet{});
        Timers::stop(it->second.timer_id);
        ping_callbacks_.erase(it);
      }
    }

    void send_request(IP4::addr dest_ip, icmp4::Type type, uint8_t code,
      icmp_func callback = nullptr, uint16_t sequence = 0);

    /** Send response without id and sequence number */
    void send_response(icmp4::Packet& req, icmp4::Type type, uint8_t code, uint8_t error = std::numeric_limits<uint8_t>::max());

    /**
     *  Responding to a ping (echo) request
     *  Called from receive-method
     */
    void ping_reply(icmp4::Packet&);

  }; //< class ICMPv4

} //< namespace net

#endif //< NET_IP4_ICMPv4_HPP
