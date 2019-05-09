// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
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
#ifndef NET_DHCP_DHCPD_HPP
#define NET_DHCP_DHCPD_HPP

#include <net/dhcp/message.hpp>
#include <net/dhcp/record.hpp>  // Status and Record
#include <net/udp/udp.hpp>
#include <map>

namespace net {
namespace dhcp {

  class DHCP_exception : public std::runtime_error {
    using runtime_error::runtime_error;
  };

  class DHCPD {
    static const uint32_t DEFAULT_LEASE = 86400;      // seconds. 1 day = 86 400 seconds.
    static const uint32_t DEFAULT_MAX_LEASE = 345600; // seconds. 4 days = 345 600 seconds.
    static const uint8_t  DEFAULT_PENDING = 30;
    static const uint8_t  MAX_NUM_OPTIONS = 30;       // max number of options in a message from a client

  public:
    DHCPD(UDP& udp, ip4::Addr pool_start, ip4::Addr pool_end,
      uint32_t lease = DEFAULT_LEASE, uint32_t max_lease = DEFAULT_MAX_LEASE, uint8_t pending = DEFAULT_PENDING);

    ~DHCPD() {
      socket_.close();
    }

    // open on DHCP client port
    void listen();

    void add_record(const Record& record)
    { records_.push_back(record); }

    bool record_exists(const Record::byte_seq& client_id) const noexcept;

    int get_record_idx(const Record::byte_seq& client_id) const noexcept;

    int get_record_idx_from_ip(ip4::Addr ip) const noexcept;

    ip4::Addr broadcast_address() const noexcept
    { return server_id_ | ( ~ netmask_); }

    ip4::Addr network_address(ip4::Addr ip) const noexcept  // x.x.x.0
    { return ip & netmask_; }

    // Getters

    ip4::Addr server_id() const noexcept
    { return server_id_; }

    ip4::Addr netmask() const noexcept
    { return netmask_; }

    ip4::Addr router() const noexcept
    { return router_; }

    ip4::Addr dns() const noexcept
    { return dns_; }

    uint32_t lease() const noexcept
    { return lease_; }

    uint32_t max_lease() const noexcept
    { return max_lease_; }

    uint8_t pending() const noexcept
    { return pending_; }

    ip4::Addr pool_start() const noexcept
    { return pool_start_; }

    ip4::Addr pool_end() const noexcept
    { return pool_end_; }

    const std::map<ip4::Addr, Status>& pool() const noexcept
    { return pool_; }

    const std::vector<Record>& records() const noexcept
    { return records_; }

    // Setters

    void set_server_id(ip4::Addr server_id)
    { server_id_ = server_id; }

    void set_netmask(ip4::Addr netmask)
    { netmask_ = netmask; }

    void set_router(ip4::Addr router)
    { router_ = router; }

    void set_dns(ip4::Addr dns)
    { dns_ = dns; }

    void set_lease(uint32_t lease)
    { lease_ = lease; }

    void set_max_lease(uint32_t max_lease)
    { max_lease_ = max_lease; }

    void set_pending(uint8_t pending)
    { pending_ = pending; }

  private:
    UDP::Stack& stack_;
    udp::Socket& socket_;
    ip4::Addr pool_start_, pool_end_;
    std::map<ip4::Addr, Status> pool_;

    ip4::Addr server_id_;
    ip4::Addr netmask_, router_, dns_;
    uint32_t lease_, max_lease_;
    uint8_t pending_;               // How long to consider an offered address in the pending state (seconds)
    std::vector<Record> records_;   // Temp - Instead of persistent storage

    bool valid_pool(ip4::Addr start, ip4::Addr end) const;
    void init_pool();
    void update_pool(ip4::Addr ip, Status new_status);

    void resolve(const Message* msg);
    void handle_request(const Message* msg);
    void verify_or_extend_lease(const Message* msg);
    void offer(const Message* msg);
    void inform_ack(const Message* msg);
    void request_ack(const Message* msg);
    void nak(const Message* msg);

    bool valid_options(const Message* msg) const;
    Record::byte_seq get_client_id(const Message* msg) const;
    ip4::Addr get_requested_ip_in_opts(const Message* msg) const;
    ip4::Addr get_remote_netmask(const Message* msg) const;
    ip4::Addr inc_addr(ip4::Addr ip) const
    { return ip4::Addr{htonl(ntohl(ip.whole) + 1)}; }
    bool on_correct_network(const Message* msg) const;

    void clear_offered_ip(ip4::Addr ip);
    void clear_offered_ips();

    void print(const Message* msg) const;
  };

} // < namespace dhcp
} // < namespace net

#endif
