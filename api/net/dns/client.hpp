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

#ifndef NET_DNS_CLIENT_HPP
#define NET_DNS_CLIENT_HPP

#include <net/inet.hpp>
#include <net/dns/dns.hpp>
#include <net/ip4/udp.hpp>
#include <util/timer.hpp>
#include <map>
#include <unordered_map>

namespace net
{
  class DNSClient
  {
  public:
    using Stack           = IP4::Stack;
    using Resolve_handler = Stack::resolve_func<IP4>;

    static Timer::duration_t DEFAULT_RESOLVE_TIMEOUT; // 5s, client.cpp

    DNSClient(Stack& stk)
      : socket_(stk.udp().bind())
    {
      // Parse received data on this socket as Responses
      socket_.on_read({this, &DNSClient::receive_response});
    }

    /**
     * @func  a delegate that provides a hostname and its address, which is 0 if the
     * name @hostname was not found. Note: Test with INADDR_ANY for a 0-address.
     **/
    void resolve(IP4::addr dns_server,
                 const std::string& hostname,
                 Resolve_handler func);

    ~DNSClient()
    {
      socket_.close();
    }

  private:
    UDPSocket& socket_;
    std::map<std::string, IP4::addr> cache;
    std::map<IP4::addr, std::string> rev_cache;

    void receive_response(IP4::addr, UDP::port_t, const char* data, size_t);

    struct Request
    {
      DNS::Request request;
      Resolve_handler callback;
      Timer timer;

      Request(DNS::Request req, Resolve_handler cb)
        : request{std::move(req)},
          callback{std::move(cb)},
          timer({this, &Request::finish})
      {
        start_timeout(DEFAULT_RESOLVE_TIMEOUT);
      }

      void finish()
      { callback(request.getFirstIP4()); }

      void start_timeout(Timer::duration_t timeout)
      { timer.start(timeout); }

    }; // < struct Request

    std::unordered_map<DNS::Request::id_t, Request> requests_;
  };
}

#endif
