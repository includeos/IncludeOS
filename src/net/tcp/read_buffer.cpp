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
  : buf(new_shared_buffer(capacity)),
    start{seq}, hole{0}
{}

size_t Read_buffer::insert(const seq_t seq, const uint8_t* data, size_t len, bool push)
{
  assert(buf != nullptr && "Buffer seems to be stolen, make sure to renew()");
  assert(seq >= start && "The sequence number cannot be before start");

  // get the relative sequence number (the diff)
  size_t rel = seq - start;
  assert(rel < capacity() && "No point trying to write at or above the end");

  // avoid writing above size by shrinking len
  len = std::min(capacity() - rel, len);

  // fill/add hole
  hole += (rel >= buf->size()) ? rel - buf->size() : -len;
  assert(hole >= 0 && "A hole cannot have a negative depth..");

  // add data to the buffer at the relative position
  if (rel == buf->size()) {
    std::copy(data, data + len, std::back_inserter(*buf));
    assert(buf->size() >= len);
  }
  else {
    if (rel + len > buf->size()) buf->resize(rel + len);
    std::copy(data, data + len, &buf->at(rel));
  }

  if (push) push_seen = true;
  return len;
}

void Read_buffer::reset(const seq_t seq)
{
  start = seq;
  hole = 0;
  push_seen = false;
  if (buf.unique()) buf->clear();
  else buf = new_shared_buffer(buf->capacity());
}

__attribute__((weak))
int Read_buffer::deserialize_from(void*) { return 0; }
__attribute__((weak))
int Read_buffer::serialize_to(void*) const { return 0; }

}
}
