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
#include <net/ip4/ip4.hpp>
#include <map>

namespace net
{
  class DNSClient
  {
  public:
    using Stack = IP4::Stack;

    DNSClient(Stack& stk)
      : stack(stk)  {}

    /**
     * @func  a delegate that provides a hostname and its address, which is 0 if the
     * name @hostname was not found. Note: Test with INADDR_ANY for a 0-address.
     **/
    void resolve(IP4::addr dns_server,
                 const std::string& hostname,
                 Stack::resolve_func<IP4> func);

  private:
    Stack& stack;
    std::map<std::string, IP4::addr> cache;
    std::map<IP4::addr, std::string> rev_cache;
  };
}

#endif
