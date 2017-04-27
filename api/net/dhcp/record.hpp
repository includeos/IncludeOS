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
#ifndef NET_DHCP_RECORD_HPP
#define NET_DHCP_RECORD_HPP

#include <cstdint>
#include <vector>
#include <net/ip4/addr.hpp>

namespace net {
namespace dhcp {

  enum class Status {
    AVAILABLE,
    OFFERED,      // IP has been offered to another client and is awaiting a DHCPREQUEST for updated status
    //RELEASED,   // Previously allocated IP to a client (reuse)
    IN_USE
  };

  // Temp - Instead of database (persistent storage)
  class Record {

  public:
    using byte_seq = std::vector<uint8_t>;

    Record() {}

    Record(const byte_seq& client_id, const ip4::Addr& ip, Status status, int64_t lease_start, uint32_t lease_duration)
    : client_id_{client_id}, ip_{ip}, status_{status}, lease_start_{lease_start}, lease_duration_{lease_duration} {}

    // Getters

    const byte_seq& client_id() const noexcept
    { return client_id_; }

    const ip4::Addr& ip() const noexcept
    { return ip_; }

    const Status& status() const noexcept
    { return status_; }

    uint32_t lease_start() const noexcept
    { return lease_start_; }

    uint32_t lease_duration() const noexcept
    { return lease_duration_; }

    // Setters

    void set_client_id(const byte_seq& client_id)
    { client_id_ = client_id; }

    void set_ip(ip4::Addr ip)
    { ip_ = ip; }

    void set_status(Status status)
    { status_ = status; }

    void set_lease_start(int64_t lease_start)
    { lease_start_ = lease_start; }

    void set_lease_duration(uint32_t lease_duration)
    { lease_duration_ = lease_duration; }

  private:
    byte_seq client_id_;
    ip4::Addr ip_;
    Status status_;
    int64_t lease_start_;           // For now: RTC::now()
    uint32_t lease_duration_;

    // TODO
    // Save client's config parameters as well - if using Status::RELEASED
    // std::vector<Option> options_;

    // T1 and T2
    // Client T1: The time at which the client enters the RENEWING state and attempts to contact the server that originally issued the
    // client's network address.
    // Client T2: The time at which the client enters the REBINDING state and attempts to contact any server.
    // T1 MUST be earlier than T2, which, in turn, MUST be earlier than the time at which the client's lease will expire.
    // T1 defaults to (0.5 * duration_of_lease)
    // T2 defaults to (0.875 * duration_of_lease)
    // T1 and T2 should be chosen with some random fuzz around a fixed value, to avoid synchronization of client reacquisition
    // A client MAY choose to renew or extend its lease prior to T1. The server MAY choose to extend the client's lease according to
    // policy set by the network administrator. The server SHOULD return T1 and T2, and their values SHOULD be adjusted from their
    // original values to take account of the time remaining on the lease
  };

} // < namespace dhcp
} // < namespace net

#endif
