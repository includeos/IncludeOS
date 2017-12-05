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

Read_buffer::Read_buffer(const size_t capacity, const seq_t startv)
  : buf(tcp::construct_buffer()),
    start{startv}, hole{0}
{
  buf->reserve(capacity);
}

size_t Read_buffer::insert(const seq_t seq, const uint8_t* data, size_t len, bool push)
{
  assert(buf != nullptr && "Buffer seems to be stolen, make sure to renew()");

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
    buf->insert(buf->end(), data, data + len);
  }
  else {
    if (rel + len > buf->size()) buf->resize(rel + len);
    __builtin_memcpy(buf->data() + rel, data, len);
  }

  if (push) push_seen = true;
  return len;
}

void Read_buffer::reset(const seq_t seq)
{
  this->reset(seq, buf->capacity());
}

void Read_buffer::reset(const seq_t seq, const size_t capacity)
{
  start = seq;
  hole = 0;
  push_seen = false;
  reset_buffer_if_needed(capacity);
}

void Read_buffer::reset_buffer_if_needed(const size_t capacity)
{
  // if the buffer isnt unique, create a new one
  if (buf.use_count() != 1)
  {
    buf = tcp::construct_buffer();
    buf->reserve(capacity);
    return;
  }
  // from here on the buffer is ours only
  buf->clear();
  const auto bufcap = buf->capacity();
  if (UNLIKELY(capacity < bufcap))
  {
    buf->shrink_to_fit();
    buf->reserve(capacity);
  }
  else if (UNLIKELY(capacity != bufcap))
  {
    buf->reserve(capacity);
  }
}

__attribute__((weak))
int Read_buffer::deserialize_from(void*) { return 0; }
__attribute__((weak))
int Read_buffer::serialize_to(void*) const { return 0; }

}
}
