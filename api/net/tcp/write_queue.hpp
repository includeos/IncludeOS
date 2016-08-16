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

#pragma once
#ifndef NET_TCP_WRITE_QUEUE_HPP
#define NET_TCP_WRITE_QUEUE_HPP

#include <debug>
#include <delegate>
#include <deque>
#include "write_buffer.hpp"


namespace net {
namespace tcp {

/*
  Write Queue containig WriteRequests from user.
  Stores requests until they are fully acknowledged;
  this will make it possible to retransmit
*/
class WriteQueue {
public:
  /*
    Callback when a write is sent by the TCP
    - Supplied on asynchronous write
  */
  using WriteCallback = delegate<void(size_t)>;

  using WriteRequest = std::pair<WriteBuffer, WriteCallback>;

public:
  WriteQueue() : q(), current_(0) {}

  /*
    Acknowledge n bytes from the write queue.
    If a Request is fully acknowledged, release from queue
    and "step back".
  */
  void acknowledge(size_t bytes) {
    debug2("<WriteQueue> Acknowledge %u bytes\n",bytes);
    while(bytes and !q.empty())
    {
      auto& buf = q.front().first;

      bytes -= buf.acknowledge(bytes);
      if(buf.done()) {
        q.pop_front();
        current_--;
        debug("<WriteQueue> Acknowledge done, current-- [%u]\n", current_);
      }
    }
  }

  bool empty() const
  { return q.empty(); }

  size_t size() const
  { return q.size(); }

  auto current() const
  { return current_; }

  auto bytes_total() const {
    uint32_t n = 0;
    for(auto& it : q)
      n += it.first.length();
    return n;
  }

  auto bytes_remaining() const {
    uint32_t n = 0;
    for(auto& it : q)
      n += it.first.remaining;
    return n;
  }

  auto bytes_unacknowledged() const {
    uint32_t n = 0;
    for(auto& it : q)
      n += it.first.length() - it.first.acknowledged;
    return n;
  }

  /*
    If the queue has more data to send
  */
  bool remaining_requests() const
  { return !q.empty() and q.back().first.remaining; }

  /*
    The current buffer to write from.
    Can be in the middle/back of the queue due to unacknowledged buffers in front.
  */
  const WriteBuffer& nxt()
  { return q.at(current_).first; }

  /*
    The oldest unacknowledged buffer. (Always in front)
  */
  const WriteBuffer& una()
  { return q.at(0).first; }

  /*
    Advances the queue forward.
    If current buffer finishes; exec user callback and step to next.
  */
  void advance(size_t bytes) {

    auto& buf = q.at(current_).first;
    buf.advance(bytes);

    debug2("<WriteQueue> Advance: bytes=%u off=%u rem=%u ack=%u\n",
      bytes, buf.offset, buf.remaining, buf.acknowledged);

    if(!buf.remaining) {
      // make sure to advance current before callback is made,
      // but after index (current) is received.
      q.at(current_++).second(buf.offset);
      debug("<WriteQueue> Advance: Done (%u) current++ [%u]\n",
        buf.offset, current_);
    }
  }

  /*
    Add a request to the back of the queue.
    If the queue was empty/finished, point current to the new request.
  */
  void push_back(const WriteRequest& wr) {
    debug("<WriteQueue> Inserted WR: off=%u rem=%u ack=%u, current=%u, size=%u\n",
      wr.first.offset, wr.first.remaining, wr.first.acknowledged, current_, size());
    q.push_back(wr);
  }

  /*
    Remove all write requests from queue and signal how much was written for each request.
  */
  void reset() {
    while(!q.empty()) {
      auto& req = q.front();
      // only give callbacks on request who hasnt finished writing
      // (others has already been called)
      if(req.first.remaining > 0)
        req.second(req.first.offset);
      q.pop_front();
    }
    current_ = 0;
    debug("<WriteQueue::reset> Reset\n");
  }

private:
  std::deque<WriteRequest> q;
  /* Current element (index) */
  uint32_t current_;


}; // < WriteQueue

} // < namespace tcp
} // < namespace net

#endif // < NET_TCP_WRITE_QUEUE_HPP
