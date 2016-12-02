#include <net/inet4>
#include <net/tcp/connection_states.hpp>
#include "serialize_tcp.hpp"

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

void Connection::deserialize_from(void* addr)
{
  auto* area = (serialized_tcp*) addr;
  
  //this->local_port_ = area->local_port;
  //this->remote_     = area->remote;
  this->cb          = area->tcb;
  this->state_      = area->to_state(area->state_now);
  this->prev_state_ = area->to_state(area->state_prev);
  this->rtx_attempt_ = area->rtx_att;
  this->syn_rtx_    = area->syn_rtx;
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

int Connection::serialize_to(void* addr)
{
  auto* area = (serialized_tcp*) addr;
  
  area->local_port = this->local_port();
  area->remote     = this->remote();
  area->tcb        = this->cb;
  area->state_now  = area->to_state(this->state_);
  area->state_prev = area->to_state(this->prev_state_);
  area->rtx_att    = this->rtx_attempt_;
  area->syn_rtx    = this->syn_rtx_;
  
  return sizeof(serialized_tcp);
}
