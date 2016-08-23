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
#include <net/tcp/socket.hpp>

using namespace net::tcp;

CASE("Creating a Socket without arguments is empty")
{
  Socket socket;
  const Address addr { 0,0,0,0 };
  EXPECT( socket.is_empty() );
  EXPECT( socket.address() == addr );
  EXPECT( socket.address() == 0 );
  EXPECT( socket.port() == 0 );

  const std::string expected_str {"0.0.0.0:0"};
  EXPECT( socket.to_string() == expected_str );
}

CASE("Creating a Socket with arguments is not empty")
{
  const Address addr { 10,0,0,42 };
  const port_t port = 80;

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
  EXPECT( sock1 < sock3 );
  EXPECT( sock2 < sock3 );
  EXPECT( sock3 > sock1 );
}

