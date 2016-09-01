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

#include <sstream>

#include <gsl/gsl_assert>
#include <net/tcp/listener.hpp>
#include <net/tcp/tcp.hpp>

using namespace net::tcp;

Listener::Listener(TCP& host, port_t port)
  : host_(host), port_(port), syn_queue_()
{
  on_accept_      = AcceptCallback::from<Listener, &Listener::default_on_accept>(this);
  on_connect_     = ConnectCallback::from<Listener, &Listener::default_on_connect>(this);
}

bool Listener::default_on_accept(Socket) {
  return true;
}

void Listener::default_on_connect(Connection_ptr) {

}

Socket Listener::local() const {
  return {host_.address(), port_};
}

void Listener::segment_arrived(Packet_ptr packet) {
  debug2("<Listener::segment_arrived> Received packet: %s\n",
    packet->to_string().c_str());
  // if it's an reply to any of our half-open connections
  if(!packet->isset(SYN))
  {
    // if there is a connection waiting for the packet
    for(auto conn : syn_queue_) // take a copy to avoid conn->segment_arrived to make a reference invalid
    {
      if(conn->remote() == packet->source())
      {
        debug("<Listener::segment_arrived> Found packet receiver: %s\n",
          conn->to_string().c_str());
        conn->segment_arrived(packet);
        debug2("<Listener::segment_arrived> Connection done handling segment\n");
        return;
      }
    }
  }
  // if it's a new attempt (SYN)
  else
  {
    // Stat increment number of connection attempts
    host_.connection_attempts_++;

    // if we don't like this client, do nothing
    if(! on_accept_(packet->source()) )
      return;

    // remove oldest connection if queue is full
    debug2("<Listener::segment_arrived> SynQueue: %u\n", syn_queue_.size());
    if(syn_queue_full())
    {
      debug2("<Listener::segment_arrived> Queue is full\n");
      Expects(not syn_queue_.empty());
      debug("<Listener::segment_arrived> Connection %s dropped to make room for new connection\n",
        syn_queue_.back()->to_string().c_str());

      syn_queue_.pop_back();
    }

    auto& conn = *(syn_queue_.emplace(syn_queue_.begin(),
      std::make_shared<Connection>( host_, port_, packet->source() )));
    // Call Listener::connected when Connection is connected
    conn->on_connect(ConnectCallback::from<Listener, &Listener::connected>(this));
    conn->_on_cleanup(CleanupCallback::from<Listener, &Listener::remove>(this));
    // Open connection
    conn->open(false);
    Ensures(conn->is_listening());
    debug("<Listener::segment_arrived> Connection %s created\n",
      conn->to_string().c_str());
    conn->segment_arrived(packet);
    debug2("<Listener::segment_arrived> Connection done handling segment\n");
    return;
  }
  debug2("<Listener::segment_arrived> No receipent\n");
}

void Listener::remove(Connection_ptr conn) {
  debug2("<Listener::remove> Try remove %s\n", conn->to_string().c_str());
  auto it = syn_queue_.begin();
  while(it != syn_queue_.end())
  {
    if((*it) == conn)
    {
      syn_queue_.erase(it);
      debug("<Listener::remove> %s removed.\n", conn->to_string().c_str());
      return;
    }
    it++;
  }
}

void Listener::connected(Connection_ptr conn) {
  debug("<Listener::connected> %s connected\n", conn->to_string().c_str());
  remove(conn);
  Expects(conn->is_connected());
  host_.add_connection(conn);
  on_connect_(conn);
}

/**
 * [Port] (SynQueue size)
 *    <SynQueue entries>
 */

std::string Listener::to_string() const {
  std::stringstream ss;
  ss << "Port [ " << port_ << " ] " << " SynQueue ( " <<  syn_queue_.size() << " ) ";

  for(auto& conn : syn_queue_)
    ss << "\n\t" << conn->to_string();

  return ss.str();
}
