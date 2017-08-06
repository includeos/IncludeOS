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
#include <net/tcp/sack.hpp>

#include <iostream>
std::ostream& operator<< (std::ostream& out, const net::tcp::sack::Entries& ent) {
  for (auto el : ent) {
    out << el << "\n";
  }
  return out;
}

net::tcp::sack::Entries expected(
  net::tcp::sack::Block first = net::tcp::sack::Block(),
  net::tcp::sack::Block second = net::tcp::sack::Block(),
  net::tcp::sack::Block third = net::tcp::sack::Block())
{
  return {{first, second, third}};
}

CASE("SACK List Array implementation [RFC 2018]")
{
  using namespace std;
  using namespace net::tcp::sack;

  // RFC 2018 example
  /*
    Case 2:  The first segment is dropped but the remaining 7 are
    received.

     Upon receiving each of the last seven packets, the data
     receiver will return a TCP ACK segment that acknowledges
     sequence number 5000 and contains a SACK option specifying
     one block of queued data:

         Triggering    ACK      Left Edge   Right Edge
         Segment

         5000         (lost)
         5500         5000     5500       6000
         6000         5000     5500       6500
         6500         5000     5500       7000
         7000         5000     5500       7500
         7500         5000     5500       8000
         8000         5000     5500       8500
         8500         5000     5500       9000
  */
  List<Array_list<9>> sack_list;
  Entries entries;

  // 5000     5500       6000
  entries = sack_list.recv_out_of_order(5500, 500);
  EXPECT(entries == expected({5500,6000}));

  // 5000     5500       6500
  entries = sack_list.recv_out_of_order(6000, 500);
  EXPECT(entries == expected({5500,6500}));

  // 5000     5500       7000
  entries = sack_list.recv_out_of_order(6500, 500);
  EXPECT(entries == expected({5500,7000}));

  // 5000     5500       7500
  entries = sack_list.recv_out_of_order(7000,500);
  EXPECT(entries == expected({5500,7500}));

  // 5000     5500       8000
  entries = sack_list.recv_out_of_order(7500,500);
  EXPECT(entries == expected({5500,8000}));

  // 5000     5500       8500
  entries = sack_list.recv_out_of_order(8000,500);
  EXPECT(entries == expected({5500,8500}));

  // 5000     5500       9000
  entries = sack_list.recv_out_of_order(8500,500);
  EXPECT(entries == expected({5500, 9000}));


  /*
    Case 3:  The 2nd, 4th, 6th, and 8th (last) segments are
    dropped.

    The data receiver ACKs the first packet normally.  The
    third, fifth, and seventh packets trigger SACK options as
    follows:

        Triggering  ACK    First Block   2nd Block     3rd Block
        Segment            Left   Right  Left   Right  Left   Right
                           Edge   Edge   Edge   Edge   Edge   Edge

        5000       5500
        5500       (lost)
        6000       5500    6000   6500
        6500       (lost)
        7000       5500    7000   7500   6000   6500
        7500       (lost)
        8000       5500    8000   8500   7000   7500   6000   6500
        8500       (lost)
  */
  sack_list = List<Array_list<9>>();

  // 5500    6000   6500
  entries = sack_list.recv_out_of_order(6000, 500);
  EXPECT(entries == expected({6000,6500}));

  // 5500    7000   7500   6000   6500
  entries = sack_list.recv_out_of_order(7000, 500);
  EXPECT(entries == expected({7000,7500}, {6000,6500}));

  // 5500    8000   8500   7000   7500   6000   6500
  entries = sack_list.recv_out_of_order(8000, 500);
  EXPECT(entries == expected({8000,8500}, {7000,7500}, {6000,6500}));

  /*
    Suppose at this point, the 4th packet is received out of order.
    (This could either be because the data was badly misordered in the
    network, or because the 2nd packet was retransmitted and lost, and
    then the 4th packet was retransmitted). At this point the data
    receiver has only two SACK blocks to report.  The data receiver
    replies with the following Selective Acknowledgment:

        Triggering  ACK    First Block   2nd Block     3rd Block
        Segment            Left   Right  Left   Right  Left   Right
                           Edge   Edge   Edge   Edge   Edge   Edge

        6500       5500    6000   7500   8000   8500
  */

  for(auto& b : sack_list.impl.blocks)
    std::cout << b << "\n";
  // 6000   7500   8000   8500
  entries = sack_list.recv_out_of_order(6500, 500);

  for(auto& b : sack_list.impl.blocks)
    std::cout << b << "\n";

  EXPECT(entries == expected({6000,7500}, {8000,8500}));

  /*
    Suppose at this point, the 2nd segment is received.  The data
    receiver then replies with the following Selective Acknowledgment:

        Triggering  ACK    First Block   2nd Block     3rd Block
        Segment            Left   Right  Left   Right  Left   Right
                           Edge   Edge   Edge   Edge   Edge   Edge

        5500       7500    8000   8500
  */
  auto result = sack_list.new_valid_ack(5500 + 500);
  for(auto& b : sack_list.impl.blocks)
    std::cout << b << "\n";
  EXPECT(result.entries == expected({8000,8500}));
  EXPECT(result.bytes_freed == 7500-6000);

}

