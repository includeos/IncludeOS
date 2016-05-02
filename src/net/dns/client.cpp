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

#include <net/dns/client.hpp>

#include <net/ip4/udp.hpp>
#include <net/dns/dns.hpp>

namespace net
{
  void DNSClient::resolve(IP4::addr dns_server, const std::string& hostname, Stack::resolve_func<IP4> func)
  {
    auto& sock = stack.udp().bind();

    // create DNS request
    DNS::Request request;
    auto*  data = new char[256];
    size_t len  = request.create(data, hostname);

    // send request to DNS server
    sock.sendto(dns_server, DNS::DNS_SERVICE_PORT, data, len);
    delete[] data;

    // wait for response
    // FIXME: WE DO NOT CHECK TRANSACTION IDS HERE (yet), GOD HELP US ALL
    sock.on_read(
    [this, hostname, request, func]
    (IP4::addr, UDP::port_t, const char* data, size_t) mutable
    {
      // original request ID = this->id;
      request.parseResponse(data);
      
      // fire onResolve event
      func(request.getFirstIP4());
    });
  }
}
