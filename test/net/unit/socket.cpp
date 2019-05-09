// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

#include <common.cxx>
#include <net/socket.hpp>
#include <net/tcp/common.hpp>

using namespace net;

CASE("Creating a Socket without arguments is empty")
{
  Socket socket;
  const Socket::Address addr { ip6::Addr::addr_any };
  EXPECT( socket.is_empty() );
  EXPECT( socket.address() == addr );
  EXPECT( socket.address().is_any() );
  EXPECT( socket.port() == 0 );

  const std::string expected_str {"[0:0:0:0:0:0:0:0]:0"};
  EXPECT( socket.to_string() == expected_str );
}

CASE("Creating a Socket with arguments is not empty")
{
  const Socket::Address addr {10,0,0,42};
  const Socket::port_t port = 80;

  Socket socket { addr, 80 };

  EXPECT_NOT( socket.is_empty() );
  EXPECT( socket.address() == addr );
  EXPECT( socket.port() == port);

  const std::string expected_str{"10.0.0.42:80"};
  EXPECT( socket.to_string() == expected_str );
}

CASE("Sockets can be compared to each other")
{
  Socket sock1 { { 10,0,0,42 }, 80 };
  Socket sock2 { { 10,0,0,42 }, 8080 };
  Socket sock3 { { 192,168,0,1 }, 80 };

  EXPECT_NOT( sock1 == sock2 );
  EXPECT_NOT( sock1 == sock3 );
  EXPECT( sock2 != sock3 );

  const Socket temp { { 10,0,0,42 }, 80 };
  EXPECT( sock1 == temp );

  const Socket empty;
  EXPECT_NOT( sock1 == empty );

  EXPECT( sock1 < sock2 );
  EXPECT( sock1 > sock3 );
  EXPECT( sock2 > sock3 );
  EXPECT( sock3 < sock1 );
}

#include <map>
CASE("Sockets can be used in a map")
{
  Socket sock_any{ip4::Addr{0}, 68};

  std::map<Socket, Socket> sockets;
  auto it = sockets.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(sock_any),
    std::forward_as_tuple(sock_any));

  EXPECT(it.second);

  Socket sock6{ip6::Addr{"2001:840:f001:4b52::42"}, 53622};

  auto v6 = sock_any.address().v6();
  printf("%x %x %x %x\n", v6.i32[0], v6.i32[1], v6.i32[2], v6.i32[3]);
  v6 = sock6.address().v6();
  printf("%x %x %x %x\n", v6.i32[0], v6.i32[1], v6.i32[2], v6.i32[3]);

  auto search = sockets.find(sock_any);
  EXPECT(search->first == sock_any);
  EXPECT(search->first != sock6);

  EXPECT(sock_any != sock6);

  EXPECT(sock_any < sock6);
  EXPECT(not (sock6 < sock_any));

  EXPECT(sock_any <= sock6);

  EXPECT(not (sock_any > sock6));

  EXPECT(sock6 > sock_any);
  EXPECT(sock6 >= sock_any);

  EXPECT(!(sock_any == sock6));

  search = sockets.find(sock6);
  EXPECT(search == sockets.end());

  it = sockets.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(sock6),
    std::forward_as_tuple(sock6));

  EXPECT(it.second);
}
