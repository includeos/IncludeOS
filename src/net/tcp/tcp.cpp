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

//#define TCP_DEBUG 1
#ifdef TCP_DEBUG
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif

#include <net/tcp/tcp.hpp>
#include <net/inet>
#include <net/inet_common.hpp> // checksum
#include <statman>
#include <rtc> // nanos_now (get_ts_value)
#include <net/tcp/packet4_view.hpp>
#include <net/tcp/packet6_view.hpp>

using namespace std;
using namespace net;
using namespace net::tcp;

TCP::TCP(IPStack& inet, bool smp_enable) :
  inet_{inet},
  listeners_(),
  connections_(),
  total_bufsize_{default_total_bufsize},
  mempool_{total_bufsize_},
  min_bufsize_{default_min_bufsize}, max_bufsize_{default_max_bufsize},
  ports_(inet.tcp_ports()),
  writeq(),
  max_seg_lifetime_{default_msl},       // 30s
  win_size_{default_ws_window_size},    // 8096*1024
  wscale_{default_window_scaling},      // 5
  timestamps_{default_timestamps},      // true
  sack_{default_sack},                  // true
  dack_timeout_{default_dack_timeout},  // 40ms
  max_syn_backlog_{default_max_syn_backlog} // 64
{
  Expects(wscale_ <= 14 && "WScale factor cannot exceed 14");
  Expects(win_size_ <= 0x40000000 && "Invalid size");

  this->cpu_id = SMP::cpu_id();
  this->smp_enabled = smp_enable;
  std::string stat_prefix;
  if (this->smp_enabled == false)
  {
    inet.on_transmit_queue_available({this, &TCP::process_writeq});
    stat_prefix = inet.ifname();
  }
  else
  {
    SMP::global_lock();
    inet.on_transmit_queue_available({this, &TCP::smp_process_writeq});
    SMP::global_unlock();
    stat_prefix = inet.ifname() + ".cpu" + std::to_string(this->cpu_id);
  }
  bytes_rx_ = &Statman::get().create(Stat::UINT64, stat_prefix + ".tcp.rx").get_uint64();
  bytes_tx_ = &Statman::get().create(Stat::UINT64, stat_prefix + ".tcp.tx").get_uint64();
  packets_rx_ = &Statman::get().create(Stat::UINT64, stat_prefix + ".tcp.packets_rx").get_uint64();
  packets_tx_ = &Statman::get().create(Stat::UINT64, stat_prefix + ".tcp.packets_tx").get_uint64();
  incoming_connections_ = &Statman::get().create(Stat::UINT64, stat_prefix + ".tcp.conn_incoming").get_uint64();
  outgoing_connections_ = &Statman::get().create(Stat::UINT64, stat_prefix + ".tcp.conn_outgoing").get_uint64();
  connection_attempts_ = &Statman::get().create(Stat::UINT64, stat_prefix + ".tcp.conn_attempts").get_uint64();
  packets_dropped_ = &Statman::get().create(Stat::UINT32, stat_prefix + ".tcp.dropped").get_uint32();
}

void TCP::smp_process_writeq(size_t packets)
{
  assert(SMP::cpu_id() == 0);
  assert(this->cpu_id != 0);
  SMP::add_task(
  [this, packets] () {
    this->process_writeq(packets);
  }, this->cpu_id);
  SMP::signal(this->cpu_id);
}

Listener& TCP::listen(const Socket& socket, ConnectCallback cb)
{
  bind(socket);

  auto& listener = listeners_.emplace(socket,
    std::make_unique<tcp::Listener>(*this, socket, std::move(cb))
    ).first->second;
  debug("<TCP::listen> Bound to socket %s \n", socket.to_string().c_str());
  return *listener;
}

Listener& TCP::listen(const tcp::port_t port, ConnectCallback cb, const bool ipv6_only)
{
  Socket socket{ip6::Addr::addr_any, port};
  bind(socket);

  auto ptr = std::make_shared<tcp::Listener>(*this, socket, std::move(cb), ipv6_only);
  auto& listener = listeners_.emplace(socket, ptr).first->second;

  if(not ipv6_only)
  {
    Socket ip4_sock{ip4::Addr::addr_any, port};
    bind(ip4_sock);
    Ensures(listeners_.emplace(ip4_sock, ptr).second && "Could not insert IPv4 listener");
  }

  return *listener;
}

bool TCP::close(const Socket& socket)
{
  // TODO: if the socket is ipv6 any addr it will also
  // close the ipv4 any addr due to call to Listener::close()
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
  auto addr = [&]()->auto{
    if(remote.address().is_v6())
    {
      auto dest = remote.address().v6();
      return Addr{inet_.addr6_config().get_src(dest)};
    }
    else
    {
      return Addr{inet_.ip_addr()};
    }
  }();

  create_connection(bind(addr), remote, std::move(callback))->open(true);
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
  auto addr = [&]()->auto{
    if(remote.address().is_v6())
    {
      auto dest = remote.address().v6();
      return Addr{inet_.addr6_config().get_src(dest)};
    }
    else
    {
      return Addr{inet_.ip_addr()};
    }
  }();

  auto conn = create_connection(bind(addr), remote);
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

void TCP::receive4(net::Packet_ptr ptr)
{
  auto ip4 = static_unique_ptr_cast<PacketIP4>(std::move(ptr));
  Packet4_view pkt{std::move(ip4)};

  PRINT("<TCP::receive> Recv TCP4 packet %s => %s\n",
    pkt.source().to_string().c_str(), pkt.destination().to_string().c_str());

  receive(pkt);
}

void TCP::receive6(net::Packet_ptr ptr)
{
  auto ip6 = static_unique_ptr_cast<PacketIP6>(std::move(ptr));
  Packet6_view pkt{std::move(ip6)};

  PRINT("<TCP::receive6> Recv TCP6 packet %s => %s\n",
    pkt.source().to_string().c_str(), pkt.destination().to_string().c_str());

  receive(pkt);
}

void TCP::receive(Packet_view& packet)
{
  // Stat increment packets received
  (*packets_rx_)++;
  assert(get_cpuid() == SMP::cpu_id());

  // validate some unlikely but invalid packet properties
  if (UNLIKELY(packet.src_port() == 0)) {
    drop(packet);
    return;
  }
  if (UNLIKELY(packet.validate_length() == false)) {
    drop(packet);
    return;
  }

#if !defined(DISABLE_INET_CHECKSUMS)
  // Validate checksum
  if (UNLIKELY(packet.compute_tcp_checksum() != 0)) {
    PRINT("<TCP::receive> TCP Packet Checksum %#x != %#x\n",
          packet.compute_tcp_checksum(), 0x0);
    drop(packet);
    return;
  }
#endif

  // Stat increment bytes received
  (*bytes_rx_) += packet.tcp_data_length();

  // Redirect packet to custom function
  if (packet_rerouter) {
    packet_rerouter(packet.release());
    return;
  }

  const auto dest = packet.destination();
  const Connection::Tuple tuple { dest, packet.source() };

  // Try to find the receiver
  auto conn_it = connections_.find(tuple);

  // Connection found
  if (conn_it != connections_.end()) {
    PRINT("<TCP::receive> Connection found: %s \n", conn_it->second->to_string().c_str());
    conn_it->second->segment_arrived(packet);
    return;
  }

  // No open connection found, find listener for destination
  debug("<TCP::receive> No connection found - looking for listener..\n");
  auto listener_it = find_listener(dest);

  // Listener found => Create Listener
  if (listener_it != listeners_.end()) {
    auto& listener = listener_it->second;
    PRINT("<TCP::receive> Listener found: %s\n", listener->to_string().c_str());
    listener->segment_arrived(packet);
    PRINT("<TCP::receive> Listener done with packet\n");
    return;
  }

  // Send a reset
  send_reset(packet);

  drop(packet);
}

// Show all connections for TCP as a string.
// Format: [Protocol][Recv][Send][Local][Remote][State]
string TCP::to_string() const {
  // Write all connections in a cute list.
  std::string str = "LISTENERS:\nLocal\tQueued\n";
  for(auto& listen_it : listeners_) {
    auto& l = listen_it;
    str += l.first.to_string() + "\t" + std::to_string(l.second->syn_queue_size()) + "\n";
  }
  str +=
  "\nCONNECTIONS:\nLocal\tRemote\tState\n";
  for(auto& con_it : connections_) {
    auto& c = *(con_it.second);
    str += c.local().to_string() + "\t" + c.remote().to_string() + "\t"
        + c.state().to_string() + "\n";
  }
  return str;
}

void TCP::error_report(const Error& err, Socket dest) {
  if (err.is_icmp()) {
    auto* icmp_err = dynamic_cast<const ICMP_error*>(&err);

    if (network().path_mtu_discovery() and icmp_err->is_too_big() and
      icmp_err->pmtu() >= network().minimum_MTU()) {

      /*
      Note RFC 1191 p. 14:
      TCP performance can be reduced if the sender’s maximum window size is not an exact multiple of
      the segment size in use (this is not the congestion window size, which is always a multiple of
      the segment size).  In many system (such as those derived from 4.2BSD), the segment size is often
      set to 1024 octets, and the maximum window size (the "send space") is usually a multiple of 1024
      octets, so the proper relationship holds by default.
      If PMTU Discovery is used, however, the segment size may not be a submultiple of the send space,
      and it may change during a connection; this means that the TCP layer may need to change the transmission
      window size when PMTU Discovery changes the PMTU value. The maximum window size should be set to the
      greatest multiple of the segment size (PMTU - 40) that is less than or equal to the sender’s buffer
      space size.
      */

      // Find all connections sending to this destination
      // Notify the TCP Connection that the sent packet has been dropped and needs to be retransmitted
      for (auto& conn_entry : connections_) {
        if (conn_entry.first.second == dest) {
          /*
          Note: One MUST not retransmit in response to every Datagram Too Big message, since
          a burst of several oversized segments will give rise to several such messages and hence
          several retransmissions of the same data. If the new estimated PMTU is still wrong, the
          process repeats, and there is an exponential growth in the number of superfluous segments
          sent. This means that the TCP layer must be able to recongnize when a Datagram Too Big
          message actually decreases the PMTU that it has already used to send a datagram on the
          given connection, and should ignore any other notifications.
          */

          // PMTU is maximum transmission unit including the size of the IP header, while SMSS is
          // minus the size of the IP header and minus the size of the TCP header
          auto new_smss = icmp_err->pmtu() - sizeof(ip4::Header) - sizeof(tcp::Header);

          if (conn_entry.second->SMSS() > new_smss) {
            conn_entry.second->set_SMSS(new_smss);

            // TODO Check that this works as expected:
            // Unlike a retransmission caused by a TCP retransmission timeout, a retransmission
            // caused by a Datagram Too Big message should not change the congestion window.
            // It should, however, trigger the slow-start mechanism (i.e., only one segment should
            // be retransmitted until acknowledgements begin to arrive again)

            // And retransmit latest packet that hasn't received an ACK
            // Note: Only retransmit in response to an ICMP Datagram Too Big message

            // Note:
            // Check if it is necessary to call reduce_ssthresh() (slow start)
            conn_entry.second->reduce_ssthresh();
            conn_entry.second->retransmit();
          }
        }
      }

      // return;
    }
  }

  // TODO - Regular error reporting

}

void TCP::reset_pmtu(Socket dest, IP4::PMTU pmtu) {
  if (UNLIKELY(not network().path_mtu_discovery() or pmtu < network().minimum_MTU()))
    return;

  // Find all connections sending to this destination and update their SMSS value
  // based on the new increased pmtu
  for (auto& conn_entry : connections_) {
    if (conn_entry.first.second == dest)
      conn_entry.second->set_SMSS(pmtu - sizeof(ip4::Header) - sizeof(tcp::Header));
  }
}

void TCP::transmit(tcp::Packet_view_ptr packet)
{
  // Generate checksum.
  packet->set_tcp_checksum();

  // Stat increment bytes transmitted and packets transmitted
  (*bytes_tx_) += packet->tcp_data_length();
  (*packets_tx_)++;

  if(packet->ipv() == Protocol::IPv6) {
    network_layer_out6_(packet->release());
  }
  else {
    network_layer_out4_(packet->release());
  }
}

tcp::Packet_view_ptr TCP::create_outgoing_packet()
{
  auto packet = std::make_unique<tcp::Packet4_view>(inet_.create_ip_packet(Protocol::TCP));
  packet->init();
  return packet;
}

tcp::Packet_view_ptr TCP::create_outgoing_packet6()
{
  auto packet = std::make_unique<tcp::Packet6_view>(inet_.create_ip6_packet(Protocol::TCP));
  packet->init();
  return packet;
}

void TCP::send_reset(const tcp::Packet_view& in)
{
  // TODO: maybe worth to just swap the fields in
  // the incoming packet and send that one
  auto out = (in.ipv() == Protocol::IPv6)
    ? create_outgoing_packet6() : create_outgoing_packet();
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
  return ((RTC::nanos_now() / 1000000000ull) & 0xffffffff);
}

void TCP::drop(const tcp::Packet_view&) {
  // Stat increment packets dropped
  (*packets_dropped_)++;
  debug("<TCP::drop> Packet dropped\n");
}

bool TCP::is_bound(const Socket& socket) const
{
  auto it = ports_.find(socket.address());

  if(it == ports_.cend() and not socket.address().is_any())
    it = ports_.find(socket.address().any_addr());

  if(it != ports_.cend())
    return it->second.is_bound(socket.port());

  return false;
}

void TCP::bind(const Socket& socket)
{
  if(UNLIKELY( is_valid_source(socket.address()) == false ))
    throw TCP_error{"Cannot bind to address: " + socket.address().to_string()};

  if (UNLIKELY( is_bound(socket) ))
    throw TCP_error{"Socket is already in use: " + socket.to_string()};

  ports_[socket.address()].bind(socket.port());
}

Socket TCP::bind(const Address& addr)
{
  if(UNLIKELY( is_valid_source(addr) == false ))
    throw TCP_error{"Cannot bind to address: " + addr.to_string()};

  auto& port_util = ports_[addr];
  const auto port = port_util.get_next_ephemeral();
  // we know the port is not bound, else the above would throw
  port_util.bind(port);
  return {addr, port};
}

bool TCP::unbind(const Socket& socket)
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

bool TCP::add_connection(tcp::Connection_ptr conn)
{
  const size_t alloc_thres = max_bufsize() * Read_request::buffer_limit;
  // Stat increment number of incoming connections
  (*incoming_connections_)++;

  debug("<TCP::add_connection> Connection added %s \n", conn->to_string().c_str());
  auto resource = mempool_.get_resource();

  // Reject connection if we can't allocate memory
  if(UNLIKELY(resource == nullptr or resource->allocatable() < alloc_thres))
  {
    conn->_on_cleanup_ = nullptr;
    conn->abort();
    return false;
  }

  conn->bufalloc = std::move(resource);

  //printf("New inc conn %s allocatable=%zu\n", conn->to_string().c_str(), conn->bufalloc->allocatable());

  Expects(conn->bufalloc != nullptr);
  conn->_on_cleanup({this, &TCP::close_connection});
  return connections_.emplace(conn->tuple(), conn).second;
}

Connection_ptr TCP::create_connection(Socket local, Socket remote, ConnectCallback cb)
{
  const size_t alloc_thres = max_bufsize() * Read_request::buffer_limit;

  auto resource = mempool_.get_resource();
  // Don't create connection if we can't allocate memory
  if(UNLIKELY(resource == nullptr or resource->allocatable() < alloc_thres))
  {
    throw TCP_error{"Unable to create new connection: Not enough allocatable memory"};
  }

  // Stat increment number of outgoing connections
  (*outgoing_connections_)++;

  auto& conn = (connections_.emplace(
      Connection::Tuple{ local, remote },
      std::make_shared<Connection>(*this, local, remote, std::move(cb))
      )
    ).first->second;
  conn->_on_cleanup({this, &TCP::close_connection});
  conn->bufalloc = std::move(resource);

  //printf("New out conn %s allocatable=%zu\n", conn->to_string().c_str(), conn->bufalloc->allocatable());

  Expects(conn->bufalloc != nullptr);
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
    // packets taken in as reference
    conn->offer(packets);
  }
}

void TCP::request_offer(Connection& conn) {
  SMP::global_lock();
  auto packets = inet_.transmit_queue_available();
  SMP::global_unlock();

  debug2("<TCP::request_offer> %s requestin offer: uw=%u rem=%u\n",
    conn.to_string().c_str(), conn.usable_window(), conn.sendq_remaining());

  // Note: Must be called even if packets is 0
  // because the connectoin is responsible for requeuing itself (see Connection::offer)
  conn.offer(packets);
}


void TCP::queue_offer(Connection& conn)
{
  if(not conn.is_queued() and conn.can_send())
  {
    try {
      debug("<TCP::queue_offer> %s queued\n", conn.to_string().c_str());
      writeq.push_back(conn.retrieve_shared());
      conn.set_queued(true);
    }
    catch (std::exception& e) {
      printf("ERROR: Could not find connection for %p: %s\n",
            &conn, conn.to_string().c_str());
      throw;
    }
  }
}

tcp::Address TCP::address() const noexcept
{ return inet_.ip_addr(); }

uint16_t TCP::MSS(const Protocol ipv) const
{
  return ((ipv == Protocol::IPv6) ?
    inet_.ip6_obj().MDDS() : network().MDDS())
    - sizeof(tcp::Header);
}

IP4& TCP::network() const
{ return inet_.ip_obj(); }

bool TCP::is_valid_source(const tcp::Address& addr) const noexcept
{ return addr.is_any() or inet_.is_valid_source(addr); }

void TCP::kick()
{ process_writeq(inet_.transmit_queue_available()); }

void TCP::close_listener(tcp::Listener& listener)
{
  const auto& socket = listener.local();
  unbind(socket);
  listeners_.erase(socket);

  // if the listener is "dual-stack", make sure to clean up the
  // ip4 any addr copy as well
  if(socket.address().is_v6() and socket.address().is_any())
  {
    Socket ip4_sock{ip4::Addr::addr_any, socket.port()};
    unbind(ip4_sock);
    listeners_.erase(ip4_sock);
  }
}

TCP::Listeners::iterator TCP::find_listener(const Socket& socket)
{
  Listeners::iterator it = listeners_.find(socket);
  if(it != listeners_.end())
    return it;

  if(not socket.address().is_any())
    it = listeners_.find({socket.address().any_addr(), socket.port()});

  return it;
}

TCP::Listeners::const_iterator TCP::cfind_listener(const Socket& socket) const
{
  Listeners::const_iterator it = listeners_.find(socket);
  if(it != listeners_.cend())
    return it;

  if(not socket.address().is_any())
    it = listeners_.find({socket.address().any_addr(), socket.port()});

  return it;
}



