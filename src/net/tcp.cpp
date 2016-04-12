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

#include <net/tcp.hpp>

using namespace std;
using namespace net;


TCP::TCP(IPStack& inet) :
  inet_(inet),
  listeners_(),
  connections_(),
  write_queue(),
  MAX_SEG_LIFETIME(30s)
{
  inet.on_transmit_queue_available(transmit_avail_delg::from<TCP,&TCP::process_write_queue>(this));
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
TCP::Connection& TCP::bind(Port port) {
  auto listen_conn_it = listeners_.find(port);
  // Already a listening socket.
  if(listen_conn_it != listeners_.end()) {
    throw TCPException{"Port is already taken."};
  }
  auto& connection = (listeners_.emplace(port, Connection{*this, port})).first->second;
  debug("<TCP::bind> Bound to port %i \n", port);
  connection.open(false);
  return connection;
}

/*
  Active open a new connection to the given remote.

  @WARNING: Callback is added when returned (TCP::connect(...).onSuccess(...)),
  and open() is called before callback is added.
*/
TCP::Connection_ptr TCP::connect(Socket remote) {
  std::shared_ptr<Connection> connection = add_connection(free_port(), remote);
  connection->open(true);
  return connection;
}

/*
  Active open a new connection to the given remote.
*/
void TCP::connect(Socket remote, Connection::ConnectCallback callback) {
  auto connection = add_connection(free_port(), remote);
  connection->onConnect(callback).open(true);
}

TCP::Seq TCP::generate_iss() {
  // Do something to get a iss.
  return rand();
}

/*
  TODO: Check if there is any ports free.
*/
TCP::Port TCP::free_port() {
  if(++current_ephemeral_ == 0)
    current_ephemeral_ = 1025;
  // Avoid giving a port that is bound to a service.
  while(listeners_.find(current_ephemeral_) != listeners_.end())
    current_ephemeral_++;

  return current_ephemeral_;
}


uint16_t TCP::checksum(TCP::Packet_ptr packet) {
  // TCP header
  TCP::Header* tcp_hdr = &(packet->header());
  // Pseudo header
  TCP::Pseudo_header pseudo_hdr;

  int tcp_length = packet->tcp_length();

  pseudo_hdr.saddr.whole = packet->src().whole;
  pseudo_hdr.daddr.whole = packet->dst().whole;
  pseudo_hdr.zero = 0;
  pseudo_hdr.proto = IP4::IP4_TCP;
  pseudo_hdr.tcp_length = htons(tcp_length);

  union {
    uint32_t whole;
    uint16_t part[2];
  } sum;

  sum.whole = 0;

  // Compute sum of pseudo header
  for (uint16_t* it = (uint16_t*)&pseudo_hdr; it < (uint16_t*)&pseudo_hdr + sizeof(pseudo_hdr)/2; it++)
    sum.whole += *it;

  // Compute sum sum the actual header and data
  for (uint16_t* it = (uint16_t*)tcp_hdr; it < (uint16_t*)tcp_hdr + tcp_length/2; it++)
    sum.whole+= *it;

  // The odd-numbered case
  if (tcp_length & 1) {
    debug("<TCP::checksum> ODD number of bytes. 0-pading \n");
    union {
      uint16_t whole;
      uint8_t part[2];
    } last_chunk;
    last_chunk.part[0] = ((uint8_t*)tcp_hdr)[tcp_length - 1];
    last_chunk.part[1] = 0;
    sum.whole += last_chunk.whole;
  }

  debug2("<TCP::checksum: sum: 0x%x, half+half: 0x%x, TCP checksum: 0x%x, TCP checksum big-endian: 0x%x \n",
         sum.whole, sum.part[0] + sum.part[1], (uint16_t)~((uint16_t)(sum.part[0] + sum.part[1])), htons((uint16_t)~((uint16_t)(sum.part[0] + sum.part[1]))));

  return ~(sum.part[0] + sum.part[1]);
}

void TCP::bottom(net::Packet_ptr packet_ptr) {
  // Translate into a TCP::Packet. This will be used inside the TCP-scope.
  auto packet = std::static_pointer_cast<TCP::Packet>(packet_ptr);
  debug("<TCP::bottom> TCP Packet received - Source: %s, Destination: %s \n",
        packet->source().to_string().c_str(), packet->destination().to_string().c_str());

  // Do checksum
  if(checksum(packet)) {
    debug("<TCP::bottom> TCP Packet Checksum != 0 \n");
  }

  Connection::Tuple tuple { packet->dst_port(), packet->source() };

  // Try to find the receiver
  auto conn_it = connections_.find(tuple);
  // Connection found
  if(conn_it != connections_.end()) {
    debug("<TCP::bottom> Connection found: %s \n", conn_it->second->to_string().c_str());
    conn_it->second->segment_arrived(packet);
  }
  // No connection found
  else {
    // Is there a listener?
    auto listen_conn_it = listeners_.find(packet->dst_port());
    debug("<TCP::bottom> No connection found - looking for listener..\n");
    // Listener found => Create listening Connection
    if(listen_conn_it != listeners_.end()) {
      auto& listen_conn = listen_conn_it->second;
      debug("<TCP::bottom> Listener found: %s ...\n", listen_conn.to_string().c_str());
      auto connection = (connections_.emplace(tuple, std::make_shared<Connection>(listen_conn)).first->second);
      // Set remote
      connection->set_remote(packet->source());
      debug("<TCP::bottom> ... Creating connection: %s \n", connection->to_string().c_str());

      connection->segment_arrived(packet);
    }
    // No listener found
    else {
      drop(packet);
    }
  }
}

void TCP::process_write_queue(size_t packets) {
  // foreach connection who wants to write
  while(packets and !write_queue.empty()) {
    auto conn = write_queue.front();
    write_queue.pop();
    // try to offer if there is any doable job, and if still more to do, requeue.
    if(conn->has_doable_job() and !conn->offer(packets)) {
      debug2("TCP::process_write_queue> %s still has more to do. Re-queued.\n");
      write_queue.push(conn);
    }
    else {
      // mark the connection as not queued.
      conn->set_queued(false);
      debug2("<TCP::process_write_queue> %s Removed from queue. Size is %u\n",
             conn->to_string().c_str(), write_queue.size());
    }
  }
}

size_t TCP::send(Connection_ptr conn, Connection::WriteBuffer& buffer) {
  size_t written{0};

  if(write_queue.empty()) {
    auto packets = inet_.transmit_queue_available();
    written = conn->send(buffer, packets);
  }

  if(written < buffer.remaining and !conn->is_queued()) {
    write_queue.push(conn);
    conn->set_queued(true);
    debug2("<TCP::send> %s wrote %u bytes (%u remaining) and is Re-queued.\n",
           conn->to_string().c_str(), written, buffer.remaining-written);
  }

  return written;
}

/*
  Show all connections for TCP as a string.

  Format:
  [Protocol][Recv][Send][Local][Remote][State]

  TODO: Make sure Recv, Send, In, Out is correct and add them to output. Also, alignment?
*/
string TCP::status() const {
  // Write all connections in a cute list.
  stringstream ss;
  ss << "LISTENING SOCKETS:\n";
  for(auto listen_it : listeners_) {
    ss << listen_it.second.to_string() << "\n";
  }
  ss << "\nCONNECTIONS:\n" <<  "Proto\tRecv\tSend\tIn\tOut\tLocal\t\t\tRemote\t\t\tState\n";
  for(auto con_it : connections_) {
    auto& c = *(con_it.second);
    ss << "tcp4\t"
       << " " << "\t" << " " << "\t"
       << " " << "\t" << " " << "\t"
       << c.local().to_string() << "\t\t" << c.remote().to_string() << "\t\t"
       << c.state().to_string() << "\n";
  }
  return ss.str();
}


TCP::Connection_ptr TCP::add_connection(Port local_port, TCP::Socket remote) {
  return        (connections_.emplace(
                                      Connection::Tuple{ local_port, remote },
                                      std::make_shared<Connection>(*this, local_port, remote))
                 ).first->second;
}

void TCP::close_connection(TCP::Connection& conn) {
  debug("<TCP::close_connection> Closing connection: %s \n", conn.to_string().c_str());
  connections_.erase(conn.tuple());
  debug2("<TCP::close_connection> TCP Status: \n%s \n", status().c_str());
}

void TCP::drop(TCP::Packet_ptr) {
  //debug("<TCP::drop> Packet was dropped - no recipient: %s \n", packet->destination().to_string().c_str());
}

void TCP::transmit(TCP::Packet_ptr packet) {
  // Translate into a net::Packet_ptr and send away.
  // Generate checksum.
  packet->set_checksum(TCP::checksum(packet));
  //packet->set_checksum(checksum(packet));
  _network_layer_out(packet);
}
