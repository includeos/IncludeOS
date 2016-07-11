// License

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
  WriteQueue() : q(), current(0) {}

  /*
    Acknowledge n bytes from the write queue.
    If a Request is fully acknowledged, release from queue
    and "step back".
  */
  void acknowledge(size_t bytes) {
    debug2("<Connection::WriteQueue> Acknowledge %u bytes\n",bytes);
    while(bytes and !q.empty())
    {
      auto& buf = q.front().first;

      bytes -= buf.acknowledge(bytes);
      if(buf.done()) {
        q.pop_front();
        current--;
        debug("<Connection::WriteQueue> Acknowledge done, current-- [%u]\n", current);
      }
    }
  }

  bool empty() const
  { return q.empty(); }

  size_t size() const
  { return q.size(); }

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
  { return q.at(current).first; }

  /*
    The oldest unacknowledged buffer. (Always in front)
  */
  const WriteBuffer& una()
  { return q.front().first; }

  /*
    Advances the queue forward.
    If current buffer finishes; exec user callback and step to next.
  */
  void advance(size_t bytes) {

    auto& buf = q.at(current).first;
    buf.advance(bytes);

    debug("<WriteQueue> Advance: bytes=%u off=%u rem=%u ack=%u\n",
      bytes, buf.offset, buf.remaining, buf.acknowledged);

    if(!buf.remaining) {
      // make sure to advance current before callback is made,
      // but after index (current) is received.
      q.at(current++).second(buf.offset);
      debug("<WriteQueue> Advance: Done (%u) current++ [%u]\n",
        buf.offset, current);
    }
  }

  /*
    Add a request to the back of the queue.
    If the queue was empty/finished, point current to the new request.
  */
  void push_back(const WriteRequest& wr) {
    debug("<WriteQueue> Inserted WR: off=%u rem=%u ack=%u, current=%u, size=%u\n",
      wr.first.offset, wr.first.remaining, wr.first.acknowledged, current, size());
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
  }

private:
  std::deque<WriteRequest> q;
  /* Current element (index) */
  uint32_t current;


}; // < WriteQueue

} // < namespace net
} // < namespace tcp

#endif // < NET_TCP_WRITE_QUEUE_HPP
