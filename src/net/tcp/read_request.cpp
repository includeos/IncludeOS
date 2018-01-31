// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 Oslo and Akershus University College of Applied Sciences
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

#include <net/tcp/read_request.hpp>

namespace net {
namespace tcp {

  Read_request::Read_request(size_t size, seq_t start, ReadCallback cb)
    : callback{cb}
  {
    buffers.push_back(std::make_unique<Read_buffer>(size, start));
  }

  size_t Read_request::insert(seq_t seq, const uint8_t* data, size_t n, bool psh)
  {
    Expects(not buffers.empty());

    //printf("insert: seq=%u len=%lu\n", seq, n);
    size_t recv{0};
    while(n)
    {
      auto* buf = get_buffer(seq);

      if(UNLIKELY(buf == nullptr))
        break;

      //printf("got buffer start=%u end=%u missing=%lu\n",
      //  buf->start_seq(), buf->end_seq(), buf->missing());

      auto read = buf->insert(seq, data + recv, n, psh);

      n -= read; // subtract amount of data left to insert
      recv += read; // add to the total recieved
      seq += read; // advance the sequence number

      // flush this (and other buffers) if ready
      // (and also being the one in the front)
      while(buf->is_ready() and buf == buffers.front().get())
      {
        callback(buf->buffer());

        // this is the only one, so we can reuse it
        if(buffers.size() == 1)
        {
          buf->reset(seq);
          break;
        }
        // to make it simple, just get rid of it and have the
        // algo create a new one if needed
        // TODO: reuse it by putting in in the back
        // (need to check if there is room for more, and
        // maybe it isnt even necessary..)
        else
        {
          buffers.pop_front();
          buf = buffers.front().get();
        }
      }
    }

    Ensures(not buffers.empty());
    return recv;
  }

  Read_buffer* Read_request::get_buffer(const seq_t seq)
  {
    // There is room in a existing one
    for(auto& ptr : buffers)
    {
      if(ptr->fits(seq) > 0)
        return ptr.get();
    }

    // We can still create a new one
    if(buffers.size() < buffer_limit)
    {
      // current cap
      const auto& back = buffers.back();

      // TODO: if the gap is bigger than 1 buffer
      // we probably need to create multiple buffers,
      // ... or just decide we only support gaps of 1 buffer size.
      buffers.push_back(
        std::make_unique<Read_buffer>(back->capacity(), back->end_seq()));

      //printf("new buffer added,fits(%lu)=%lu\n",
      //  seq, buffers.back()->fits(seq));

      if(buffers.back()->fits(seq) > 0)
        return buffers.back().get();
    }

    return nullptr;
  }

  size_t Read_request::fits(const seq_t seq) const
  {
    // There is room in a existing one
    for(auto& ptr : buffers)
    {
      if(ptr->fits(seq) > 0)
        return ptr->fits(seq);
    }

    // Possible to create one (or more) to support this?
    if(buffers.size() < buffer_limit)
    {
      auto& back = buffers.back();
      const auto rel = seq - back->end_seq();
      const auto cap = back->capacity();

      if(rel < cap)
        return (cap - rel);
    }

    return 0;
  }

  size_t Read_request::size() const
  {
    size_t bytes = 0;

    for(auto& ptr : buffers)
      bytes += ptr->size();

    return bytes;
  }

  void Read_request::set_start(seq_t seq)
  {
    for(auto& ptr : buffers)
    {
      Expects(ptr->size() == 0 && "Cannot change start sequence when there already is data");
      ptr->set_start(seq);
      seq += ptr->capacity();
      Ensures(seq == ptr->end_seq());
    }
  }

  void Read_request::reset(size_t size, const seq_t seq)
  {
    Expects(not buffers.empty());

    auto it = buffers.begin();

    // get the first buffer
    auto* buf = it->get();
    // if it contains data without any holes,
    // return it to the user
    if(buf->size() > 0 and buf->missing() == 0)
    {
      callback(buf->buffer());
    }
    // reset the first buffer
    buf->reset(seq, size);
    // throw the others away
    buffers.erase(++it, buffers.end());

    Ensures(buffers.size() == 1);
  }

}
}
