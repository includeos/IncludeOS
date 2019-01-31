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
#include <util/bitops.hpp>

namespace net {
namespace tcp {

Read_buffer::Read_buffer(const seq_t startv, const size_t min, const size_t max)
  : buf(tcp::construct_buffer()),
    start{startv}, cap{max}, hole{0}
{
  Expects(util::bits::is_pow2(cap));
  Expects(util::bits::is_pow2(min));
  Expects(cap >= min);
  buf->reserve(min);
}

Read_buffer::Read_buffer(const seq_t startv, const size_t min, const size_t max,
                         const Alloc& alloc)
  : buf(tcp::construct_buffer(alloc)),
    start{startv}, cap{max}, hole{0}
{
  Expects(util::bits::is_pow2(cap));
  Expects(util::bits::is_pow2(min));
  Expects(cap >= min);
  buf->reserve(min);
}

size_t Read_buffer::insert(const seq_t seq, const uint8_t* data, size_t len, bool push)
{
  assert(buf != nullptr && "Buffer seems to be stolen, make sure to renew()");

  // get the relative sequence number (the diff)
  size_t rel = (seq_t)(seq - start);
  assert(rel < capacity() && "No point trying to write at or above the end");

  //printf("seq=%u, start=%u, rel: %lu sz=%lu\n", seq, start, rel, size());
  // avoid writing above size by shrinking len
  len = std::min(capacity() - rel, len);

  // fill/add hole
  hole += (rel >= buf->size()) ? (rel - buf->size()) : -len;
  assert(hole >= 0 && "A hole cannot have a negative depth..");

  // add data to the buffer at the relative position
  if (rel == buf->size()) {
    buf->insert(buf->end(), data, data + len);
  }
  else
  {
    if (rel + len > buf->size())
      buf->resize(rel + len);

    __builtin_memcpy(buf->data() + rel, data, len);
  }

  if (push) push_seen = true;

  return len;
}

void Read_buffer::reset(const seq_t seq)
{
  this->reset(seq, capacity());
}

void Read_buffer::reset(const seq_t seq, const size_t capacity)
{
  start = seq;
  hole = 0;
  push_seen = false;
  cap = capacity;
  reset_buffer_if_needed();
}

void Read_buffer::reset_buffer_if_needed()
{
  // current buffer cap
  const auto bufcap = buf->capacity();

  // buffer is only ours
  if(buf.unique())
  {
    buf->clear();
  }
  // if the buffer isnt unique, create a new one
  else
  {
    buf = tcp::construct_buffer(buf->get_allocator());
  }

  // This case is when we need a small buffer in front of
  // another buffer due to SACK
  if (UNLIKELY(cap < bufcap))
  {
    buf->shrink_to_fit();
    buf->reserve(cap);
  }
  // if not we just reserve the same capacity as we had the last time
  else
  {
    buf->reserve(bufcap);
  }
}

__attribute__((weak))
int Read_buffer::deserialize_from(void*) { return 0; }
__attribute__((weak))
int Read_buffer::serialize_to(void*) const { return 0; }

}
}
