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

#pragma once
#ifndef NET_TCP_WRITE_QUEUE_HPP
#define NET_TCP_WRITE_QUEUE_HPP

#include <debug>
#include <delegate>
#include <deque>
#include "common.hpp"

namespace net {
namespace tcp {

/*
  Write Queue containig WriteRequests from user.
  Stores requests until they are fully acknowledged;
  this will make it possible to retransmit

  [UNA][xxx][xxx][NXT][---][---]
  ^               ^
  sent but        not sent (or partial)
  not acked
  (or partial)
*/
class Write_queue {
public:
  using WriteCallback = delegate<void(size_t)>;
  using WriteBuffer   = buffer_t;

public:
  explicit Write_queue(WriteCallback cb = nullptr);

  /*
    Add a request to the back of the queue.
    If the queue was empty/finished, point current to the new request.
  */
  void push_back(buffer_t wr) {
    debug2("<WriteQueue> Inserted WR: size=%u, current=%u, size=%u\n",
      (uint32_t) wr->size(), current_, (uint32_t) size());
    q.push_back(std::move(wr));
  }

  /*
    Advances the queue forward.
    If current buffer finishes; exec user callback and step to next.
  */
  void advance(size_t bytes);

  /*
    Acknowledge n bytes from the write queue.
    If a Request is fully acknowledged, release from queue
    and "step back".
  */
  void acknowledge(size_t bytes);

  /*
    Remove all write requests from queue and signal how much was written for each request.
  */
  void reset();

  /*
    The current buffer to write from.
    Can be in the middle/back of the queue due to unacknowledged buffers in front.
  */
  const WriteBuffer& nxt() const
  { return q.at(current_); }

  /*
    The oldest unacknowledged buffer. (Always in front)
  */
  const WriteBuffer& una() const
  { return q.at(0); }

  void on_write(WriteCallback cb)
  { on_write_ = std::move(cb); }

  bool empty() const
  { return q.empty(); }

  size_t size() const
  { return q.size(); }

  auto current() const
  { return current_; }

  auto offset() const
  { return offset_; }

  auto acked() const
  { return acked_; }

  const uint8_t* nxt_data() const
  { return &q.at(current_)->at(offset_); }

  auto nxt_rem() const
  { return q.at(current_)->size() - offset_; }

  auto bytes_total() const {
    uint32_t n = 0;
    for(auto& it : q)
      n += it->size();
    return n;
  }

  uint32_t bytes_remaining() const;

  uint32_t bytes_unacknowledged() const;

  /*
    If the queue has more data to send
  */
  bool has_remaining_requests() const
  { return current_ < q.size(); }


  // ???
  int deserialize_from(void*);
  int serialize_to(void*) const;

private:
  std::deque<WriteBuffer> q;
  /* Current element (index) */
  uint32_t current_;
  /* Offset of nxt() */
  uint32_t offset_;
  /* Acknowledged of una() */
  uint32_t acked_;
  /* Write callback - invoked when a buffer is fully sent */
  WriteCallback on_write_;


}; // < WriteQueue

} // < namespace tcp
} // < namespace net

#endif // < NET_TCP_WRITE_QUEUE_HPP
