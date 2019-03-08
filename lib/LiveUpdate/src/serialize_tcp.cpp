// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 IncludeOS AS, Oslo, Norway
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
/**
 * Master thesis
 * by Alf-Andre Walla 2016-2017
 *
**/
#include <net/inet>
#include <net/tcp/connection_states.hpp>
#include <net/tcp/stream.hpp>
#include "serialize_tcp.hpp"
#include "liveupdate.hpp"
#include "storage.hpp"
#include <cstring>
#include <unordered_set>

using namespace net::tcp;
static std::unordered_set<net::Inet*> slumbering_ip4;

Connection::State* serialized_tcp::to_state(int8_t state) const
{
  switch (state) {
  case 0:  return &Connection::Closed::instance();
  case 1:  return &Connection::Listen::instance();
  case 2:  return &Connection::SynSent::instance();
  case 3:  return &Connection::SynReceived::instance();
  case 4:  return &Connection::Established::instance();
  case 5:  return &Connection::FinWait1::instance();
  case 6:  return &Connection::FinWait2::instance();
  case 7:  return &Connection::CloseWait::instance();
  case 8:  return &Connection::Closing::instance();
  case 9:  return &Connection::LastAck::instance();
  case 10: return &Connection::TimeWait::instance();
  }
  throw std::runtime_error("to_state: Unknown TCP state");
}
int8_t serialized_tcp::to_state(Connection::State* state) const
{
  if (state == &Connection::Closed::instance()) return 0;
  if (state == &Connection::Listen::instance()) return 1;
  if (state == &Connection::SynSent::instance()) return 2;
  if (state == &Connection::SynReceived::instance()) return 3;
  if (state == &Connection::Established::instance()) return 4;
  if (state == &Connection::FinWait1::instance()) return 5;
  if (state == &Connection::FinWait2::instance()) return 6;
  if (state == &Connection::CloseWait::instance()) return 7;
  if (state == &Connection::Closing::instance()) return 8;
  if (state == &Connection::LastAck::instance()) return 9;
  if (state == &Connection::TimeWait::instance()) return 10;
  throw std::runtime_error("to_state: Unknown TCP state");
}

struct read_buffer
{
  uint32_t  seq;
  int32_t   head;
  int32_t   hole;
  bool      push;
  size_t    capacity;

  size_t size() const noexcept {
    return head;
  }

  char vla[0];
};

struct write_buffer
{
  size_t   length;
  char     vla[0];
};

struct serialized_writeq
{
  uint32_t current;
  uint32_t offset;
  uint32_t acked;
  size_t   buffers;

  char     vla[0];
};


int Write_queue::deserialize_from(void* addr)
{
  auto* writeq = (serialized_writeq*) addr;
  this->current_ = writeq->current;
  this->offset_  = writeq->offset;
  this->acked_   = writeq->acked;

  /// restore write buffers
  int len = 0;
  int total = writeq->buffers;

  while (total--)
  {
    auto* current = (write_buffer*) &writeq->vla[len];
    // header
    len += sizeof(write_buffer);

    // insert shared buffer into write queue
    this->q.emplace_back(net::tcp::construct_buffer());

    // copy data
    auto wbuf = this->q.back();
    auto* source = &writeq->vla[len];
    std::copy(source, source + current->length, std::back_inserter(*wbuf));
    len += current->length;
  }
  return sizeof(serialized_writeq) + len;
}

int Read_buffer::deserialize_from(void* addr)
{
  const auto& readq = *reinterpret_cast<read_buffer*>(addr);

  // start (seq) and capacity is set in constructor
  this->hole      = readq.hole;
  this->push_seen = readq.push;

  if(readq.size() > 0) {
    std::copy(readq.vla, readq.vla + readq.size(), std::back_inserter(*buffer()));
  }

  return sizeof(read_buffer) + readq.size();
}

void Connection::deserialize_from(void* addr)
{
  if(this->VERSION != serialized_tcp::VERSION)
    throw std::runtime_error{"TCP Serialization version mismatch"};

  auto* area = (serialized_tcp*) addr;

  /// restore TCP stuff
  //this->local_     = area->local;
  //this->remote_    = area->remote;
  this->cb           = area->tcb;
  this->state_       = area->to_state(area->state_now);
  this->prev_state_  = area->to_state(area->state_prev);
  this->rttm         = area->rttm;
  this->rtx_attempt_ = area->rtx_att;
  this->syn_rtx_     = area->syn_rtx;
  this->queued_      = area->queued;
  this->fast_recovery_  = area->fast_recovery;
  this->reno_fpack_seen = area->reno_fpack_seen;
  this->limited_tx_  = area->limited_tx;
  this->dup_acks_    = area->dup_acks;
  this->highest_ack_ = area->highest_ack;
  this->prev_highest_ack_ = area->prev_highest_ack;
  // new:
  this->last_acked_ts_ = area->last_acked_ts;
  this->dack_          = area->dack;
  this->last_ack_sent_ = area->last_ack_sent;

  /// restore writeq from VLA
  int writeq_len = this->writeq.deserialize_from(area->vla);
  if (sendq_size() > 0)
  {
    /// we have to retransmit because of magic
    slumbering_ip4.insert(&this->host_.stack());
  }

  // Assign new memory resource from TCP
  this->bufalloc = host_.mempool_.get_resource();

  /// restore read queue
  auto* readq = (read_buffer*) &area->vla[writeq_len];
  if (readq->capacity)
  {
    read_request = std::make_unique<Read_request>(readq->seq, readq->capacity, host_.max_bufsize(), bufalloc.get());
    read_request->front().deserialize_from(readq);
  }

  if (area->rtx_is_running) {
    this->rtx_start();
  }

  //printf("READ: %u  SEND: %u  REMAIN: %u  STATE: %s\n",
  //    readq_size(), sendq_size(), sendq_remaining(), cb.to_string().c_str());
}
Connection_ptr deserialize_connection(void* addr, net::TCP& tcp)
{
  auto* area = (serialized_tcp*) addr;

  auto conn = std::make_shared<Connection> (tcp, area->local, area->remote);
  conn->deserialize_from(addr);
  // add connection TCP list
  tcp.insert_connection(conn);
  return conn;
}

int  Write_queue::serialize_to(void* addr) const
{
  auto* writeq = (serialized_writeq*) addr;
  writeq->current = this->current_;
  writeq->offset  = this->offset_;
  writeq->acked   = this->acked_;
  writeq->buffers = this->q.size();

  int len = 0;
  for (auto& wbuf : this->q)
  {
    auto* current = (write_buffer*) &writeq->vla[len];

    // header
    current->length = wbuf->size();
    len += sizeof(write_buffer);

    // data
    memcpy(&writeq->vla[len], wbuf->data(), current->length);
    len += current->length;
  }
  return sizeof(serialized_writeq) + len;
}

int Read_buffer::serialize_to(void* addr) const
{
  auto& readbuf = *reinterpret_cast<read_buffer*>(addr);

  readbuf.seq   = this->start;
  readbuf.hole  = this->hole;
  readbuf.push  = this->push_seen;

  std::copy(this->buf->begin(), this->buf->end(), readbuf.vla);

  return sizeof(read_buffer) + this->size();
}

int Connection::serialize_to(void* addr) const
{
  if(this->VERSION != serialized_tcp::VERSION)
    throw std::runtime_error{"TCP Serialization version mismatch"};

  auto* area = (serialized_tcp*) addr;

  /// serialize TCP stuff
  area->local      = this->local();
  area->remote     = this->remote();
  area->tcb        = this->cb;
  area->state_now  = area->to_state(this->state_);
  area->state_prev = area->to_state(this->prev_state_);
  area->rttm       = this->rttm;
  area->rtx_att    = this->rtx_attempt_;
  area->syn_rtx    = this->syn_rtx_;
  area->dup_acks   = this->dup_acks_;
  area->queued     = this->queued_;
  area->fast_recovery    = this->fast_recovery_;
  area->reno_fpack_seen  = this->reno_fpack_seen;
  area->limited_tx = this->limited_tx_;
  area->dup_acks   = this->dup_acks_;
  area->highest_ack      = this->highest_ack_;
  area->prev_highest_ack = this->prev_highest_ack_;
  area->last_acked_ts    = this->last_acked_ts_;
  area->dack       = this->dack_;
  area->last_ack_sent= this->last_ack_sent_;

  area->rtx_is_running = this->rtx_timer.is_running();

  /// serialize write queue
  int writeq_len = this->writeq.serialize_to(area->vla);

  /// serialize read queue
  auto* readq = (read_buffer*) &area->vla[writeq_len];
  int readq_len = (read_request) ? read_request->front().serialize_to(readq) : sizeof(read_buffer);

  //printf("READ: %u  SEND: %u  REMAIN: %u  STATE: %s\n",
  //    readq_size(), sendq_size(), sendq_remaining(), cb.to_string().c_str());

  return sizeof(serialized_tcp) + writeq_len + readq_len;
}

void serialized_tcp::wakeup_ip_networks()
{
  // start all the send queues for the slumbering IP stacks
  for (auto& stack : slumbering_ip4) {
    stack->force_start_send_queues();
  }
}

/// public API ///
namespace liu
{
  void Storage::add_connection(uid id, Connection_ptr conn)
  {
    hdr.add_struct(TYPE_TCP, id,
    [&conn] (char* location) -> int {
      // return size of all the serialized data
      return conn->serialize_to(location);
    });
  }
  Connection_ptr Restore::as_tcp_connection(net::TCP& tcp) const
  {
    return deserialize_connection(ent->vla, tcp);
  }
  net::Stream_ptr Restore::as_tcp_stream   (net::TCP& tcp) const
  {
    return std::make_unique<net::tcp::Stream> (as_tcp_connection(tcp));
  }
}
