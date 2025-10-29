// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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
#include <net/ip4/cidr.hpp>

using namespace net::ip4;

CASE("Creating Cidrs from Addr and mask")
{
  const Cidr cidr1{Addr(0), 32};
  const Cidr cidr2{Addr(0), 24};
  const Cidr cidr3{Addr(0), 0};

  const Cidr cidr4{Addr(20,30,40,50), 10};
  const Cidr cidr5{Addr(10,20,30,45), 29};

  EXPECT(cidr1.from() == Addr(0));
  EXPECT(cidr1.to() == Addr(0));
  EXPECT(cidr1.contains(Addr(0)));
  EXPECT(not cidr1.contains(Addr(0,0,0,1)));

  EXPECT(cidr2.from() == Addr(0));
  EXPECT(cidr2.to() == Addr(0,0,0,255));
  EXPECT(cidr2.contains(Addr(0,0,0,50)));
  EXPECT(not cidr2.contains(Addr(0,0,1,0)));

  EXPECT(cidr3.from() == Addr(0));
  EXPECT(cidr3.to() == Addr(255,255,255,255));
  EXPECT(cidr3.contains(Addr(150,140,30,10)));
  EXPECT(cidr3.contains(Addr(0,0,0,1)));
  EXPECT(cidr3.contains(Addr(255,255,255,254)));

  EXPECT(cidr4.from() == Addr(20,0,0,0));
  EXPECT(cidr4.to() == Addr(20,63,255,255));
  EXPECT(cidr4.contains(Addr(20,40,40,255)));
  EXPECT(cidr4.contains(Addr(20,63,255,254)));
  EXPECT(cidr4.contains(Addr(20,30,40,50)));
  EXPECT(not cidr4.contains(Addr(20,64,0,0)));
  EXPECT(not cidr4.contains(Addr(19,255,255,255)));

  EXPECT(cidr5.from() == Addr(10,20,30,40));
  EXPECT(cidr5.to() == Addr(10,20,30,47));
  EXPECT(cidr5.contains(Addr(10,20,30,41)));
  EXPECT(cidr5.contains(Addr(10,20,30,46)));
  EXPECT(not cidr5.contains(Addr(10,20,30,39)));
  EXPECT(not cidr5.contains(Addr(10,20,30,48)));
}

CASE("Creating Cidrs from uint8_ts")
{
  const Cidr cidr1{10,0,0,30,32};
  const Cidr cidr2{0,0,0,0,32};
  const Cidr cidr3{192,168,0,1,12};
  const Cidr cidr4{192,168,0,1,30};

  EXPECT(cidr1.from() == Addr(10,0,0,30));
  EXPECT(cidr1.to() == Addr(10,0,0,30));
  EXPECT(cidr1.contains(Addr(10,0,0,30)));
  EXPECT(not cidr1.contains(Addr(10,0,0,29)));
  EXPECT(not cidr1.contains(Addr(10,0,0,31)));

  EXPECT(cidr2.from() == Addr(0,0,0,0));
  EXPECT(cidr2.to() == Addr(0,0,0,0));
  EXPECT(cidr2.contains(Addr(0,0,0,0)));
  EXPECT(not cidr2.contains(Addr(0,0,0,1)));

  EXPECT(cidr3.from() == Addr(192,160,0,0));
  EXPECT(cidr3.to() == Addr(192,175,255,255));
  EXPECT(cidr3.contains(Addr(192,160,0,0)));
  EXPECT(cidr3.contains(Addr(192,160,0,1)));
  EXPECT(cidr3.contains(Addr(192,175,255,254)));
  EXPECT(cidr3.contains(Addr(192,175,255,255)));
  EXPECT(not cidr3.contains(Addr(192,159,255,255)));
  EXPECT(not cidr3.contains(Addr(192,176,0,0)));

  EXPECT(cidr4.from() == Addr(192,168,0,0));
  EXPECT(cidr4.to() == Addr(192,168,0,3));
  EXPECT(cidr4.contains(Addr(192,168,0,1)));
  EXPECT(cidr4.contains(Addr(192,168,0,2)));
  EXPECT(not cidr4.contains(Addr(192,168,0,4)));
}

CASE("Creating Cidrs from string")
{
  const Cidr cidr1{"120.30.40.1/24"};
  const Cidr cidr2{"120.30.40.5/29"};
  const Cidr cidr3{"10.2.3/25"};

  EXPECT(cidr1.from() == Addr(120,30,40,0));
  EXPECT(cidr1.to() == Addr(120,30,40,255));
  EXPECT(cidr1.contains(Addr(120,30,40,1)));
  EXPECT(cidr1.contains(Addr(120,30,40,254)));
  EXPECT(cidr1.contains(Addr(120,30,40,128)));
  EXPECT(not cidr1.contains(Addr(120,30,39,255)));
  EXPECT(not cidr1.contains(Addr(120,30,41,0)));

  EXPECT(cidr2.from() == Addr(120,30,40,0));
  EXPECT(cidr2.to() == Addr(120,30,40,7));
  EXPECT(cidr2.contains(Addr(120,30,40,1)));
  EXPECT(cidr2.contains(Addr(120,30,40,6)));
  EXPECT(not cidr2.contains(Addr(120,30,39,255)));
  EXPECT(not cidr2.contains(Addr(120,30,40,8)));

  EXPECT(cidr3.from() == Addr(10,2,3,0));
  EXPECT(cidr3.to() == Addr(10,2,3,127));
  EXPECT(cidr3.contains(Addr(10,2,3,1)));
  EXPECT(cidr3.contains(Addr(10,2,3,126)));
  EXPECT(not cidr3.contains(Addr(10,2,2,255)));
  EXPECT(not cidr3.contains(Addr(10,2,3,128)));
}

CASE("Invalid Cidrs created from string should throw exception")
{
  EXPECT_THROWS(Cidr{"40.120.32.13"});
  EXPECT_THROWS(Cidr{"40.120.32.3/"});
  EXPECT_THROWS(Cidr{"40.120.32.5/a"});
  EXPECT_THROWS(Cidr{"20.120.32.5/33"});
}
