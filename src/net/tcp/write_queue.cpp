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

#include <net/tcp/write_queue.hpp>

using namespace net::tcp;

Write_queue::Write_queue(WriteCallback cb)
  : current_(0),
    offset_(0),
    acked_(0),
    on_write_(cb)
{}

void Write_queue::advance(size_t bytes)
{
  auto& buf = nxt();

  offset_ += bytes;
  assert(offset_ <= buf->size());

  debug2("<WriteQueue> Advance: bytes=%u off=%u rem=%u\n",
    bytes, offset_, (buf->size() - offset_));

  if(offset_ == buf->size())
  {
    current_++;
    offset_ = 0;

    if(on_write_)
      on_write_(buf->size());

    debug("<WriteQueue> Advance: Done (%u) current++ [%u] sz=%u\n",
      buf->size(), current_, q.size());
  }
}

void Write_queue::acknowledge(size_t bytes)
{
  debug2("<WriteQueue> Acknowledge %u bytes, ack=%u\n", bytes, acked_);
  while(bytes and !q.empty())
  {
    auto& buf = una();
    assert(buf->size() >= acked_);
    // remaining
    const auto rem = buf->size() - acked_;

    // if everything or more is acked
    if(bytes >= rem)
    {
      // subtract the bytes acked
      bytes -= rem;
      // reset acked
      acked_ = 0;
      // pop and subtract index
      q.pop_front();
      current_--;

      debug("<WriteQueue> Acknowledge done, current-- [%u] sz=%u\n", current_, q.size());
    }
    else
    {
      // add to acked
      acked_ += bytes;
      bytes = 0;
    }
  }
}

void Write_queue::reset()
{
  if(offset_ > 0 and on_write_ != nullptr)
    on_write_(offset_);

  q.clear();
  current_ = 0;
  debug("<WriteQueue::reset> Reset\n");
}

uint32_t Write_queue::bytes_remaining() const
{
  if(current_ >= q.size()) return 0;

  uint32_t n = nxt()->size() - offset_;

  for(auto i = current_ + 1; i < q.size(); ++i)
    n += q.at(i)->size();

  return n;
}

uint32_t Write_queue::bytes_unacknowledged() const
{
  if(q.empty()) return 0;

  uint32_t n = una()->size() - acked_;

  for(uint32_t i = 1; i < q.size(); ++i)
    n += q.at(i)->size();

  return n;
}

__attribute__((weak))
int Write_queue::deserialize_from(void*) { return 0; }
__attribute__((weak))
int  Write_queue::serialize_to(void*) const { return 0; }
