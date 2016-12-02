#include <net/tcp/connection.hpp>
#pragma once

struct serialized_tcp
{
  typedef net::tcp::Connection Connection;
  typedef net::tcp::port_t     port_t;
  typedef net::tcp::Socket     Socket;
  
  port_t local_port;
  Socket remote;
  Connection::TCB tcb;
  
  int8_t state_now;
  int8_t state_prev;
  
  int8_t rtx_att;
  int8_t syn_rtx;
  
  /// write buffers
  int8_t write_buffers;
  
  
  Connection::State* to_state(int8_t st);
  int8_t to_state(Connection::State* state);
};

extern std::shared_ptr<net::tcp::Connection> deserialize_connection(void* addr, net::TCP& tcp);
