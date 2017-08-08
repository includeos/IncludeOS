// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

#include <net/tcp/read_buffer.hpp>

namespace net {
namespace tcp {

Read_buffer::Read_buffer(const size_t capacity, const seq_t seq)
  : buf{new_shared_buffer(capacity)},
    cap{capacity},
    start{seq},
    head{0},
    hole{0}
{}

size_t Read_buffer::insert(const seq_t seq, const uint8_t* data, size_t len, bool push)
{
  Expects(buf != nullptr && "Buffer seems to be stolen, make sure to renew()");

  // get the relative sequence number (the diff)
  int32_t rel = (seq - start);

  Expects(rel >= 0 && "The sequence number cannot be before start");
  Expects(rel < static_cast<int64_t>(cap) && "No point trying to write at or above the end");

  // avoid writing above size by shrinking len
  len = std::min(cap - rel, len);

  // add data to the buffer at the relative position
  std::copy(data, data + len, buf.get() + rel);

  // fill/add hole
  hole += (rel >= head) ? rel - head : -len;

  Ensures(hole >= 0 && "A hole cannot have a negative depth..");

  // if sequence number advances (instead of filling hole), update head
  if(rel + static_cast<int64_t>(len) > head)
    head = rel + len;

  Ensures(head <= static_cast<int64_t>(cap) && "Head has passed the allowed size");

  if(push) push_seen = true;

  return len;
}

void Read_buffer::renew(const seq_t seq)
{
  start = seq;
  head = 0; hole = 0;
  push_seen = false;
  buf = new_shared_buffer(cap);
}

__attribute__((weak))
int Read_buffer::deserialize_from(void*) { return 0; }
__attribute__((weak))
int Read_buffer::serialize_to(void*) const { return 0; }

}
}
