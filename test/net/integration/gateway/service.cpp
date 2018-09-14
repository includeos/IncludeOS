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

#include <service>
#include <net/inet>

using namespace net;

int pongs = 0;

void verify() {
  static int i = 0;
  if (++i == 8) printf("SUCCESS\n");
}

#include <net/tcp/tcp_conntrack.hpp>

void Service::start()
{
  static auto& eth0 = Inet::stack<0>();

  static auto& eth1 = Inet::stack<1>();

  static auto& host1 = Inet::stack<2>();

  static auto& host2 = Inet::stack<3>();

  eth0.conntrack()->tcp_in = net::tcp::tcp4_conntrack;

  INFO("Ping", "host1 => host2 (%s)", host2.ip_addr().to_string().c_str());
  host1.icmp().ping(host2.ip_addr(), [](auto reply) {
    CHECKSERT(reply, "Got pong from %s", host2.ip_addr().to_string().c_str());
    pongs++;
    verify();
  }, 1);

  INFO("Ping", "host1 => eth0 (%s)", eth0.ip_addr().to_string().c_str());
  host1.icmp().ping(eth0.ip_addr(), [](auto reply) {
    CHECKSERT(reply, "Got pong from %s", eth0.ip_addr().to_string().c_str());
    pongs++;
    verify();
  }, 1);

  INFO("Ping", "host1 => eth1 (%s)", eth1.ip_addr().to_string().c_str());
  host1.icmp().ping(eth1.ip_addr(), [](auto reply) {
    CHECKSERT(reply, "Got pong from %s", eth1.ip_addr().to_string().c_str());
    pongs++;
    verify();
  }, 1);

  INFO("Ping", "host2 => host1 (%s)", host1.ip_addr().to_string().c_str());
  host2.icmp().ping(host1.ip_addr(), [](auto reply) {
    CHECKSERT(reply, "Got pong from %s", host1.ip_addr().to_string().c_str());
    pongs++;
    verify();
  }, 1);

  INFO("TCP", "host2 => listen:5000");
  host2.tcp().listen(5000, [](auto /*conn*/) {
  });

  INFO("TCP", "host1 => host2 (%s:%i)", host2.ip_addr().to_string().c_str(), 5000);
  host1.tcp().connect({host2.ip_addr(), 5000}, [](auto conn) {
    CHECKSERT(conn, "Connection established with %s:%i", host2.ip_addr().to_string().c_str(), 5000);
    verify();
  });

  INFO("TCP", "host1 => eth0 (%s:%i)", eth0.ip_addr().to_string().c_str(), 5001);
  host1.tcp().connect({eth0.ip_addr(), 5001}, [](auto conn) {
    CHECKSERT(conn, "Connection established with %s:%i", eth0.ip_addr().to_string().c_str(), 5001);
    verify();
  });

  INFO("TCP", "host2 => listen:1337");
  host2.tcp().listen(1337, [](auto) {});
  INFO("TCP", "host1 => eth0 (%s:%i)", eth0.ip_addr().to_string().c_str(), 1337);
  host1.tcp().connect({eth0.ip_addr(), 1337}, [](auto conn) {
    CHECKSERT(conn, "Connection established with %s:%i", eth0.ip_addr().to_string().c_str(), 1337);
    verify();
  });

  INFO("UDP", "eth0 => listen:4444");
  eth0.udp().bind(4444).on_read([&](auto addr, auto, const char*, size_t) {
    CHECKSERT(addr == eth1.ip_addr(), "Received UDP data from eth1");
    verify();
  });
  std::string udp_data{"yolo"};
  eth1.udp().bind().sendto(eth0.ip_addr(), 3333, udp_data.data(), udp_data.size());

}
