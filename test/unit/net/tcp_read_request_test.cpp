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
#include <net/tcp/read_request.hpp>

CASE("Operating with out of order data")
{
  using namespace net::tcp;
  const size_t BUFSZ = 4096;
  const seq_t SEQ_START = 0;
  const size_t SEGSZ = 1460;

  uint8_t data[SEGSZ];
  seq_t seq = SEQ_START;
  int no_reads;
  size_t fits, insert;

  Read_request::ReadCallback read_cb = [&](auto buf) mutable {
    no_reads++;
  };

  auto req = std::make_unique<Read_request>(seq, BUFSZ, BUFSZ);
  req->on_read_callback = read_cb;
  no_reads = 0;

  // Insert hole, first missing
  fits = req->fits(seq + SEGSZ);
  EXPECT(fits == BUFSZ - SEGSZ);
  insert = req->insert(seq + SEGSZ, data, std::min(SEGSZ, fits));
  EXPECT(insert == SEGSZ);

  // Insert in order
  fits = req->fits(seq);
  EXPECT(fits == BUFSZ);
  insert = req->insert(seq, data, std::min(SEGSZ, fits));
  EXPECT(insert == SEGSZ);
  // Increase sequence number
  seq += insert;
  seq += SEGSZ; // for the one added earlier

  // Insert 2 ahead of current pos (result in new buffer) (with PUSH)
  fits = req->fits(seq + (SEGSZ*2));
  EXPECT(fits == BUFSZ - ((SEGSZ*4) - BUFSZ)); // ...
  insert = req->insert(seq + (SEGSZ*2), data, std::min(SEGSZ, fits), true);
  EXPECT(insert == SEGSZ);

  // In order, filling the last bytes in the first buffer
  fits = req->fits(seq);
  EXPECT(fits == BUFSZ - (SEGSZ*2));
  insert = req->insert(seq, data, std::min(SEGSZ, fits));
  EXPECT(insert == fits);
  EXPECT(no_reads == 1); // first buffer should be cleared
  seq += insert; // add the inserted bytes

  auto remaining = SEGSZ - insert;
  EXPECT(remaining < SEGSZ);
  fits = req->fits(seq);
  EXPECT(fits == BUFSZ);
  insert = req->insert(seq, data, remaining);
  EXPECT(insert == remaining);
  seq += insert;

  // Fill the missing packet
  fits = req->fits(seq);
  EXPECT(fits == BUFSZ - remaining);
  insert = req->insert(seq, data, std::min(SEGSZ, fits));
  EXPECT(no_reads == 2);
  seq += insert;
  seq += SEGSZ; // for the one added earlier

  EXPECT(seq == (uint32_t)(SEQ_START + SEGSZ*5));
}
