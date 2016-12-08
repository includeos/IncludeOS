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

#define DEBUG
#define DEBUG2

#include <net/tcp/tcp.hpp>
#include <net/tcp/packet.hpp>
#include <statman>

using namespace std;
using namespace net;
using namespace net::tcp;

TCP::TCP(IPStack& inet) :
  bytes_rx_{Statman::get().create(Stat::UINT64, inet.ifname() + ".tcp.bytes_rx").get_uint64()},
  bytes_tx_{Statman::get().create(Stat::UINT64, inet.ifname() + ".tcp.bytes_tx").get_uint64()},
  packets_rx_{Statman::get().create(Stat::UINT64, inet.ifname() + ".tcp.packets_rx").get_uint64()},
  packets_tx_{Statman::get().create(Stat::UINT64, inet.ifname() + ".tcp.packets_tx").get_uint64()},
  incoming_connections_{Statman::get().create(Stat::UINT64, inet.ifname() + ".tcp.incoming_connections").get_uint64()},
  outgoing_connections_{Statman::get().create(Stat::UINT64, inet.ifname() + ".tcp.outgoing_connections").get_uint64()},
  connection_attempts_{Statman::get().create(Stat::UINT64, inet.ifname() + ".tcp.connection_attempts").get_uint64()},
  packets_dropped_{Statman::get().create(Stat::UINT32, inet.ifname() + ".tcp.packets_dropped").get_uint32()},
  inet_(inet),
  listeners_(),
  connections_(),
  writeq(),
  MAX_SEG_LIFETIME(30s)
{
  inet.on_transmit_queue_available({this, &TCP::process_writeq});
}

/*
  Note: There is different approaches to how to handle listeners & connections.
  Need to discuss and decide for the best one.

  Best solution(?):
  Preallocate a pool with listening connections.
  When threshold is reach, remove/add new ones, similar to TCP window.

  Current solution:
  Simple.
*/
Listener& TCP::bind(const port_t port) {
  // Already a listening socket.
  Listeners::const_iterator it = listeners_.find(port);
  if(it != listeners_.cend()) {
    throw TCPException{"Port is already taken."};
  }
  auto& listener = listeners_.emplace(port,
    std::make_unique<tcp::Listener>(*this, port)
    ).first->second;
  debug("<TCP::bind> Bound to port %i \n", port);
  return *listener;

  /*auto& listener = (listeners_.emplace(std::piecewise_construct,
    std::forward_as_tuple(port),
    std::forward_as_tuple(*this, port))
    ).first->second;
  debug("<TCP::bind> Bound to port %i \n", port);

  return listener;*/
}

bool TCP::unbind(const port_t port) {
  auto it = listeners_.find(port);
  if(LIKELY(it != listeners_.end())) {
    auto listener = std::move(it->second);
    listener->close();
    Ensures(listeners_.find(port) == listeners_.end());
    return true;
  }
  return false;
}

/*
  Active open a new connection to the given remote.

  @WARNING: Callback is added when returned (TCP::connect(...).onSuccess(...)),
  and open() is called before callback is added.
*/
Connection_ptr TCP::connect(Socket remote) {
  auto port = next_free_port();
  auto connection = add_connection(port, remote);
  connection->open(true);
  return connection;
}

/*
  Active open a new connection to the given remote.
*/
void TCP::connect(Socket remote, Connection::ConnectCallback callback) {
  auto port = next_free_port();
  auto connection = add_connection(port, remote);
  connection->on_connect(callback).open(true);
}

void TCP::insert_connection(Connection_ptr conn)
{
  connections_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(conn->local_port(), conn->remote()),
      std::forward_as_tuple(conn));
}


seq_t TCP::generate_iss() {
  // Do something to get a iss.
  return rand();
}

/*
  TODO: Check if there is any ports free.
*/
port_t TCP::next_free_port() {

  current_ephemeral_ = (current_ephemeral_ == 0) ? current_ephemeral_ + 1025 : current_ephemeral_ + 1;

  // Avoid giving a port that is bound to a service.
  while(listeners_.find(current_ephemeral_) != listeners_.end())
    current_ephemeral_++;

  return current_ephemeral_;
}

/*
  Expensive look up if port is in use.
*/
bool TCP::port_in_use(const port_t port) const {
  if(listeners_.find(port) != listeners_.end())
    return true;

  for(auto conn : connections_) {
    if(conn.first.first == port)
      return true;
  }
  return false;
}

uint16_t TCP::checksum(const tcp::Packet& packet)
{
  short tcp_length = packet.tcp_length();

  Pseudo_header pseudo_hdr;
  pseudo_hdr.saddr.whole = packet.src().whole;
  pseudo_hdr.daddr.whole = packet.dst().whole;
  pseudo_hdr.zero = 0;
  pseudo_hdr.proto = IP4::IP4_TCP;
  pseudo_hdr.tcp_length = htons(tcp_length);

  union Sum{
    uint32_t whole;
    uint16_t part[2];
  } sum;

  sum.whole = 0;

  // Compute sum of pseudo header
  for (uint16_t* it = (uint16_t*)&pseudo_hdr; it < (uint16_t*)&pseudo_hdr + sizeof(pseudo_hdr)/2; it++)
    sum.whole += *it;

  // Compute sum of header and data
  Header* tcp_hdr = &packet.tcp_header();
  for (uint16_t* it = (uint16_t*)tcp_hdr; it < (uint16_t*)tcp_hdr + tcp_length/2; it++)
    sum.whole+= *it;

  // The odd-numbered case
  bool odd = (tcp_length & 1);
  sum.whole += (odd) ? ((uint8_t*)tcp_hdr)[tcp_length - 1] << 16 : 0;

  sum.whole = (uint32_t)sum.part[0] + sum.part[1];
  sum.part[0] += sum.part[1];

  return ~sum.whole;
}

void TCP::bottom(net::Packet_ptr packet_ptr) {
  // Stat increment packets received
  packets_rx_++;

  // Translate into a TCP::Packet. This will be used inside the TCP-scope.
  auto packet = static_unique_ptr_cast<net::tcp::Packet>(std::move(packet_ptr));
  debug2("<TCP::bottom> TCP Packet received - Source: %s, Destination: %s \n",
        packet->source().to_string().c_str(), packet->destination().to_string().c_str());

  // Stat increment bytes received
  bytes_rx_ += packet->tcp_data_length();

  // Validate checksum
  if (UNLIKELY(checksum(*packet) != 0)) {
    debug("<TCP::bottom> TCP Packet Checksum != 0 \n");
    drop(*packet);
    return;
  }

  Connection::Tuple tuple { packet->dst_port(), packet->source() };

  // Try to find the receiver
  auto conn_it = connections_.find(tuple);

  // Connection found
  if (conn_it != connections_.end()) {
    debug("<TCP::bottom> Connection found: %s \n", conn_it->second->to_string().c_str());
    conn_it->second->segment_arrived(std::move(packet));
    return;
  }

  // No open connection found, find listener on port
  Listeners::iterator listener_it = listeners_.find(packet->dst_port());
  debug("<TCP::bottom> No connection found - looking for listener..\n");
  // Listener found => Create listening Connection
  if (LIKELY(listener_it != listeners_.end())) {
    auto& listener = listener_it->second;
    debug("<TCP::bottom> Listener found: %s\n", listener->to_string().c_str());
    listener->segment_arrived(std::move(packet));
    debug2("<TCP::bottom> Listener done with packet\n");
    return;
  }

  drop(*packet);
}

void TCP::process_writeq(size_t packets) {
  debug("<TCP::process_writeq> size=%u p=%u\n", writeq.size(), packets);
  // foreach connection who wants to write
  while(packets and !writeq.empty()) {
    debug("<TCP::process_writeq> Processing writeq size=%u, p=%u\n", writeq.size(), packets);
    auto conn = writeq.front();
    writeq.pop_back();
    conn->offer(packets);
    conn->set_queued(false);
  }
}

size_t TCP::send(Connection_ptr conn, const char* buffer, size_t n) {
  size_t written{0};
  auto packets = inet_.transmit_queue_available();

  debug2("<TCP::send> Send request for %u bytes\n", n);

  if(packets > 0) {
    written += conn->send(buffer, n, packets);
  }
  // if connection still can send (means there wasn't enough packets)
  // only requeue if not already queued
  if(!packets and conn->can_send() and !conn->is_queued()) {
    debug("<TCP::send> %s queued\n", conn->to_string().c_str());
    writeq.push_back(conn);
    conn->set_queued(true);
  }

  return written;
}

/*
  Show all connections for TCP as a string.

  Format:
  [Protocol][Recv][Send][Local][Remote][State]

  TODO: Make sure Recv, Send, In, Out is correct and add them to output. Also, alignment?
*/
string TCP::to_string() const {
  // Write all connections in a cute list.
  stringstream ss;
  ss << "LISTENERS:\n" << "Port\t" << "Queued\n";
  for(auto& listen_it : listeners_) {
    auto& l = listen_it.second;
    ss << l->port() << "\t" << l->syn_queue_size() << "\n";
  }
  ss << "\nCONNECTIONS:\n" <<  "Proto\tRecv\tSend\tIn\tOut\tLocal\t\t\tRemote\t\t\tState\n";
  for(auto& con_it : connections_) {
    auto& c = *(con_it.second);
    ss << "tcp4\t"
       << " " << "\t" << " " << "\t"
       << " " << "\t" << " " << "\t"
       << c.local().to_string() << "\t\t" << c.remote().to_string() << "\t\t"
       << c.state().to_string() << "\n";
  }
  return ss.str();
}

Connection_ptr TCP::add_connection(port_t local_port, Socket remote) {
  // Stat increment number of outgoing connections
  outgoing_connections_++;

  auto& conn = (connections_.emplace(
      Connection::Tuple{ local_port, remote },
      std::make_shared<Connection>(*this, local_port, remote))
    ).first->second;
  conn->_on_cleanup({this, &TCP::close_connection});
  return conn;
}

void TCP::add_connection(tcp::Connection_ptr conn) {
  // Stat increment number of incoming connections
  incoming_connections_++;

  debug("<TCP::add_connection> Connection added %s \n", conn->to_string().c_str());
  conn->_on_cleanup({this, &TCP::close_connection});
  connections_.emplace(conn->tuple(), conn);
}

void TCP::close_connection(tcp::Connection_ptr conn) {
  debug("<TCP::close_connection> Closing connection: %s \n", conn->to_string().c_str());
  connections_.erase(conn->tuple());
}

void TCP::close_listener(Listener& listener) {
  listeners_.erase(listener.port());
}

void TCP::drop(const tcp::Packet&) {
  // Stat increment packets dropped
  packets_dropped_++;

  debug("<TCP::drop> Packet dropped\n");
  //debug("<TCP::drop> Packet was dropped - no recipient: %s \n", packet->destination().to_string().c_str());
}

void TCP::transmit(tcp::Packet_ptr packet) {
  // Generate checksum.
  packet->set_checksum(TCP::checksum(*packet));
  debug2("<TCP::transmit> %s\n", packet->to_string().c_str());

  // Stat increment bytes transmitted and packets transmitted
  bytes_tx_ += packet->tcp_data_length();
  packets_tx_++;

  _network_layer_out(std::move(packet));
}
