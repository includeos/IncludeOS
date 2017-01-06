#include <net/inet4>
#include <net/tcp/connection_states.hpp>
#include "serialize_tcp.hpp"
#include <cstring>

using namespace net::tcp;

Connection::State* serialized_tcp::to_state(int8_t st)
{
  switch (st) {
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
  default: assert(0);
  }
}
int8_t serialized_tcp::to_state(Connection::State* state)
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
  assert(0);
}

struct write_buffer
{
  size_t   remaining;
  size_t   offset;
  size_t   acknowledged;
  bool     push;
  
  size_t   length() const noexcept { return remaining + offset; }
  
  char     vla[0];
};

struct serialized_writeq
{
  uint32_t current;
  size_t   buffers;
  
  char     vla[0];
};


void WriteQueue::deserialize_from(void* addr)
{
  auto* writeq = (serialized_writeq*) addr;
  this->current_ = writeq->current;
  
  /// restore write buffers
  int len = 0;
  int bytes = 0;
  int total = writeq->buffers;
  
  while (total--) {
    
    auto* current = (write_buffer*) &writeq->vla[len];
    // header
    len += sizeof(write_buffer);
    
    buffer_t wq_buffer { new uint8_t[current->length()], std::default_delete<uint8_t[]> () };
    // copy data
    memcpy(wq_buffer.get(), &writeq->vla[len], current->length());
    len += current->length();
    
    /// insert into write queue
    this->q.emplace_back(
        std::piecewise_construct,
        std::forward_as_tuple(wq_buffer, current->length(), current->push, current->offset),
        std::forward_as_tuple([] (int) {})); // no-op write callback
    
    this->q.back().first.acknowledged = current->acknowledged;
    assert(this->q.back().first.length() == current->length());
    bytes += current->length();
  }
}
void Connection::deserialize_from(void* addr)
{
  auto* area = (serialized_tcp*) addr;
  
  /// restore TCP stuff
  //this->local_port_ = area->local_port;
  //this->remote_     = area->remote;
  this->cb           = area->tcb;
  this->state_       = area->to_state(area->state_now);
  this->prev_state_  = area->to_state(area->state_prev);
  this->rttm         = area->rttm;
  this->rtx_attempt_ = area->rtx_att;
  this->syn_rtx_     = area->syn_rtx;
  this->dup_acks_    = area->dup_acks;
  this->queued_      = area->queued;
  /// restore writeq from VLA
  this->writeq.deserialize_from(area->vla);
  /// we have to retransmit because of magic
  //this->retransmit();
}
Connection_ptr deserialize_connection(void* addr, net::TCP& tcp)
{
  auto* area = (serialized_tcp*) addr;
  
  auto conn = std::make_shared<Connection> (tcp, area->local_port, area->remote);
  conn->deserialize_from(addr);
  // add connection TCP list
  tcp.insert_connection(conn);
  return conn;
}

int  WriteQueue::serialize_to(void* addr)
{
  auto* writeq = (serialized_writeq*) addr;
  writeq->current = this->current_;
  writeq->buffers = this->q.size();
  
  int len = 0;
  for (auto& wq : this->q) {
    
    auto* current = (write_buffer*) &writeq->vla[len];
    auto& buf = wq.first;
    
    // header
    current->remaining = buf.remaining;
    current->offset    = buf.offset;
    current->acknowledged = buf.acknowledged;
    current->push      = buf.push;
    len += sizeof(write_buffer);
    
    // data
    memcpy(&writeq->vla[len], buf.buffer.get(), current->length());
    len += current->length();
  }
  
  return sizeof(serialized_writeq) + len;
}

int Connection::serialize_to(void* addr)
{
  auto* area = (serialized_tcp*) addr;
  
  /// serialize TCP stuff
  area->local_port = this->local_port();
  area->remote     = this->remote();
  area->tcb        = this->cb;
  area->state_now  = area->to_state(this->state_);
  area->state_prev = area->to_state(this->prev_state_);
  area->rttm       = this->rttm;
  area->rtx_att    = this->rtx_attempt_;
  area->syn_rtx    = this->syn_rtx_;
  area->dup_acks   = this->dup_acks_;
  area->queued     = this->queued_;
  
  /// serialize write queue
  int writeq_len = this->writeq.serialize_to(area->vla);
  
  return sizeof(serialized_tcp) + writeq_len;
}
