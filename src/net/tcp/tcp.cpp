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

// #define DEBUG
// #define DEBUG2

#include <net/tcp/tcp.hpp>
#include <net/inet_common.hpp> // checksum
#include <statman>
#include <os> // micros_since_boot (get_ts_value)

using namespace std;
using namespace net;
using namespace net::tcp;

TCP::TCP(IPStack& inet) :
  inet_{inet},
  listeners_(),
  connections_(),
  writeq(),
  max_seg_lifetime_{default_msl},       // 30s
  win_size_{default_ws_window_size},    // 8096*1024
  wscale_{default_window_scaling},      // 5
  timestamps_{default_timestamps},      // true
  dack_timeout_{default_dack_timeout},  // 40ms
  max_syn_backlog_{default_max_syn_backlog}, // 64
  bytes_rx_{Statman::get().create(Stat::UINT64, inet.ifname() + ".tcp.bytes_rx").get_uint64()},
  bytes_tx_{Statman::get().create(Stat::UINT64, inet.ifname() + ".tcp.bytes_tx").get_uint64()},
  packets_rx_{Statman::get().create(Stat::UINT64, inet.ifname() + ".tcp.packets_rx").get_uint64()},
  packets_tx_{Statman::get().create(Stat::UINT64, inet.ifname() + ".tcp.packets_tx").get_uint64()},
  incoming_connections_{Statman::get().create(Stat::UINT64, inet.ifname() + ".tcp.incoming_connections").get_uint64()},
  outgoing_connections_{Statman::get().create(Stat::UINT64, inet.ifname() + ".tcp.outgoing_connections").get_uint64()},
  connection_attempts_{Statman::get().create(Stat::UINT64, inet.ifname() + ".tcp.connection_attempts").get_uint64()},
  packets_dropped_{Statman::get().create(Stat::UINT32, inet.ifname() + ".tcp.packets_dropped").get_uint32()}
{
  Expects(wscale_ <= 14 && "WScale factor cannot exceed 14");
  Expects(win_size_ <= 0x40000000 && "Invalid size");

  inet.on_transmit_queue_available({this, &TCP::process_writeq});
}


TCP::Port_util::Port_util()
  : ports{},
    ephemeral_(new_ephemeral_port()),
    eph_count(0)
{
  ports.set(port_ranges::DYNAMIC_END);
}

void TCP::Port_util::increment_ephemeral()
{
  if(UNLIKELY(! has_free_ephemeral() ))
    throw TCP_error{"All ephemeral ports are taken"};

  ephemeral_ = (ephemeral_ == port_ranges::DYNAMIC_END)
    ? port_ranges::DYNAMIC_START
    : ephemeral_ + 1;

  // TODO: Avoid wrap around, increment ephemeral to next free port.
  // while(is_bound(ephemeral_)) ++ephemeral_; // worst case is like 16k iterations :D
  // need a solution that checks each word of the subset (the dynamic range)
  Ensures(is_bound(ephemeral_) == false && "Hoped I wouldn't see the day..."); // this may happen...
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
Listener& TCP::listen(tcp::Socket socket, ConnectCallback cb)
{
  bind(socket);

  auto& listener = listeners_.emplace(socket,
    std::make_unique<tcp::Listener>(*this, socket, std::move(cb))
    ).first->second;
  debug("<TCP::listen> Bound to socket %s \n", socket.to_string());
  return *listener;
}

bool TCP::close(tcp::Socket socket) {
  auto it = listeners_.find(socket);
  if(it != listeners_.end())
  {
    auto listener = std::move(it->second);
    listener->close();
    Ensures(listeners_.find(socket) == listeners_.end());
    return true;
  }
  return false;
}

void TCP::connect(Socket remote, ConnectCallback callback)
{
  create_connection(bind(), remote, std::move(callback))->open(true);
}

void TCP::connect(Address source, Socket remote, ConnectCallback callback)
{
  connect(bind(source), remote, std::move(callback));
}

void TCP::connect(Socket local, Socket remote, ConnectCallback callback)
{
  bind(local);
  create_connection(local, remote, std::move(callback))->open(true);
}

Connection_ptr TCP::connect(Socket remote)
{
  auto conn = create_connection(bind(), remote);
  conn->open(true);
  return conn;
}

Connection_ptr TCP::connect(Address source, Socket remote)
{
  auto conn = create_connection(bind(source), remote);
  conn->open(true);
  return conn;
}

Connection_ptr TCP::connect(Socket local, Socket remote)
{
  bind(local);
  auto conn = create_connection(local, remote);
  conn->open(true);
  return conn;
}

void TCP::insert_connection(Connection_ptr conn)
{
  connections_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(conn->local(), conn->remote()),
      std::forward_as_tuple(conn));
}

void TCP::receive(net::Packet_ptr packet_ptr) {
  // Stat increment packets received
  packets_rx_++;

  // Translate into a TCP::Packet. This will be used inside the TCP-scope.
  auto packet = static_unique_ptr_cast<net::tcp::Packet>(std::move(packet_ptr));
  const auto dest = packet->destination();
  debug2("<TCP::receive> TCP Packet received - Source: %s, Destination: %s \n",
        packet->source().to_string().c_str(), dest.to_string().c_str());

  // Validate checksum
  if (UNLIKELY(checksum(*packet) != 0)) {
    debug("<TCP::receive> TCP Packet Checksum != 0 \n");
    drop(*packet);
    return;
  }

  // Stat increment bytes received
  bytes_rx_ += packet->tcp_data_length();

  const Connection::Tuple tuple { dest, packet->source() };

  // Try to find the receiver
  auto conn_it = connections_.find(tuple);

  // Connection found
  if (conn_it != connections_.end()) {
    debug("<TCP::receive> Connection found: %s \n", conn_it->second->to_string().c_str());
    conn_it->second->segment_arrived(std::move(packet));
    return;
  }

  // No open connection found, find listener for destination
  debug("<TCP::receive> No connection found - looking for listener..\n");
  auto listener_it = find_listener(dest);

  // Listener found => Create Listener
  if (listener_it != listeners_.end()) {
    auto& listener = listener_it->second;
    debug("<TCP::receive> Listener found: %s\n", listener->to_string().c_str());
    listener->segment_arrived(std::move(packet));
    debug2("<TCP::receive> Listener done with packet\n");
    return;
  }

  // Send a reset
  send_reset(*packet);

  drop(*packet);
}

uint16_t TCP::checksum(const tcp::Packet& packet)
{
  short length = packet.tcp_length();
  // Compute sum of pseudo-header
  uint32_t sum =
        (packet.ip_src().whole >> 16)
      + (packet.ip_src().whole & 0xffff)
      + (packet.ip_dst().whole >> 16)
      + (packet.ip_dst().whole & 0xffff)
      + (static_cast<uint8_t>(Protocol::TCP) << 8)
      + htons(length);

  // Compute sum of header and data
  const char* buffer = (char*) &packet.tcp_header();
  return net::checksum(sum, buffer, length);
}

// Show all connections for TCP as a string.
// Format: [Protocol][Recv][Send][Local][Remote][State]
string TCP::to_string() const {
  // Write all connections in a cute list.
  stringstream ss;
  ss << "LISTENERS:\n" << "Local\t" << "Queued\n";
  for(auto& listen_it : listeners_) {
    auto& l = listen_it.second;
    ss << l->local().to_string() << "\t" << l->syn_queue_size() << "\n";
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

void TCP::error_report(Error_type type, Error_code code,
  tcp::Address src_addr, tcp::port_t src_port, tcp::Address dest_addr, tcp::port_t dest_port) {
  printf("<TCP::error_report> Error %s : %s occurred when sending data to %s port %u from %s port %u\n",
    icmp4::get_type_string(type).c_str(), icmp4::get_code_string(type, code).c_str(),
    dest_addr.to_string().c_str(), dest_port, src_addr.to_string().c_str(), src_port);
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

tcp::Packet_ptr TCP::create_outgoing_packet()
{
  auto packet = static_unique_ptr_cast<net::tcp::Packet>(inet_.create_packet());
  packet->init();
  return packet;
}

void TCP::send_reset(const tcp::Packet& in)
{
  // TODO: maybe worth to just swap the fields in
  // the incoming packet and send that one
  auto out = create_outgoing_packet();
  // increase incoming SEQ and ACK by 1 and set RST + ACK
  out->set_seq(in.ack()+1).set_ack(in.seq()+1).set_flags(RST | ACK);
  // swap dest and src
  out->set_source(in.destination());
  out->set_destination(in.source());

  transmit(std::move(out));
}

seq_t TCP::generate_iss() {
  // Do something to get a iss.
  return rand();
}

uint32_t TCP::get_ts_value() const
{
  return ((OS::micros_since_boot() >> 10) & 0xffffffff);
}

void TCP::drop(const tcp::Packet&) {
  // Stat increment packets dropped
  packets_dropped_++;
  debug("<TCP::drop> Packet dropped\n");
}

bool TCP::is_bound(const Socket socket) const
{
  auto it = ports_.find(socket.address());

  if(it == ports_.cend() and socket.address() != 0)
    it = ports_.find({0});

  if(it != ports_.cend())
    return it->second.is_bound(socket.port());

  return false;
}

void TCP::bind(const Socket socket)
{
  if(UNLIKELY( is_valid_source(socket.address()) == false ))
    throw TCP_error{"Cannot bind to address: " + socket.address().to_string()};

  if (UNLIKELY( is_bound(socket) ))
    throw TCP_error{"Socket is already in use: " + socket.to_string()};

  ports_[socket.address()].bind(socket.port());
}

Socket TCP::bind(const Address addr)
{
  if(UNLIKELY( is_valid_source(addr) == false ))
    throw TCP_error{"Cannot bind to address: " + addr.to_string()};

  auto& port_util = ports_[addr];
  const auto port = port_util.get_next_ephemeral();
  // we know the port is not bound, else the above would throw
  port_util.bind(port);
  return {addr, port};
}

bool TCP::unbind(const Socket socket)
{
  auto it = ports_.find(socket.address());

  if(it != ports_.end())
  {
    auto& port_util = it->second;
    if( port_util.is_bound(socket.port()) )
    {
      port_util.unbind(socket.port());
      return true;
    }
  }
  return false;
}

void TCP::add_connection(tcp::Connection_ptr conn) {
  // Stat increment number of incoming connections
  incoming_connections_++;

  debug("<TCP::add_connection> Connection added %s \n", conn->to_string().c_str());
  conn->_on_cleanup({this, &TCP::close_connection});
  connections_.emplace(conn->tuple(), conn);
}

Connection_ptr TCP::create_connection(Socket local, Socket remote, ConnectCallback cb)
{
  // Stat increment number of outgoing connections
  outgoing_connections_++;

  auto& conn = (connections_.emplace(
      Connection::Tuple{ local, remote },
      std::make_shared<Connection>(*this, local, remote, std::move(cb))
      )
    ).first->second;
  conn->_on_cleanup({this, &TCP::close_connection});
  return conn;
}

void TCP::process_writeq(size_t packets) {
  debug2("<TCP::process_writeq> size=%u p=%u\n", writeq.size(), packets);
  // foreach connection who wants to write
  while(packets and !writeq.empty()) {
    debug("<TCP::process_writeq> Processing writeq size=%u, p=%u\n", writeq.size(), packets);
    auto conn = writeq.front();
    // remove from writeq
    writeq.pop_front();
    conn->set_queued(false);
    // ...
    conn->offer(packets);
  }
}

void TCP::request_offer(Connection& conn) {
  auto packets = inet_.transmit_queue_available();

  debug2("<TCP::request_offer> %s requestin offer: uw=%u rem=%u\n",
    conn.to_string().c_str(), conn.usable_window(), conn.sendq_remaining());

  conn.offer(packets);
}

void TCP::queue_offer(Connection_ptr conn)
{
  if(not conn->is_queued() and conn->can_send())
  {
    debug("<TCP::queue_offer> %s queued\n", conn->to_string().c_str());
    writeq.push_back(conn);
    conn->set_queued(true);
  }
}
