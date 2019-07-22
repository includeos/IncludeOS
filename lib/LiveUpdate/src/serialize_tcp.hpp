/**
 * Master thesis
 * by Alf-Andre Walla 2016-2017
 *
**/
#pragma once
#include <net/tcp/connection.hpp>

struct serialized_tcp
{
  // has to match with tcp::Connection::VERSION
  static const int VERSION = 2;

  typedef net::tcp::Connection Connection;
  typedef net::tcp::port_t     port_t;
  typedef net::Socket          Socket;

  Socket local;
  Socket remote;

  int8_t  state_now;
  int8_t  state_prev;

  Connection::TCB tcb;
  net::tcp::RTTM  rttm;

  int8_t  rtx_att;
  int8_t  syn_rtx;

  bool    queued;

  bool    fast_recovery;
  bool    reno_fpack_seen;
  bool    limited_tx;
  uint8_t dup_acks;
  uint8_t dack;

  net::tcp::seq_t highest_ack;
  net::tcp::seq_t prev_highest_ack;
  uint32_t last_acked_ts;

  net::tcp::seq_t last_ack_sent;

  bool rtx_is_running;

  /// vla for write buffers
  char   vla[0];

  /// helper functions
  Connection::State* to_state(int8_t st) const;
  int8_t to_state(Connection::State* state) const;
  static void wakeup_ip_networks();
};

extern std::shared_ptr<::net::tcp::Connection> deserialize_connection(void* addr, net::TCP& tcp);
