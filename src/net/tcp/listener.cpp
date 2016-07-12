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

#include <gsl_assert.h>
#include <net/tcp/listener.hpp>
#include <net/tcp/tcp.hpp>

using namespace net::tcp;

Listener::Listener(TCP& host, port_t port)
  : host_(host), port_(port), syn_queue_()
{
  on_accept_      = AcceptCallback::from<Listener, &Listener::default_on_accept>(this);
  on_connect_     = ConnectCallback::from<Listener, &Listener::default_on_connect>(this);
}


void Listener::segment_arrived(Packet_ptr packet) {
  debug("<Listener::segment_arrived> Received packet: %s\n",
    packet->to_string().c_str());
  // if it's an ACK for our SYN-ACK
  if(!packet->isset(SYN))
  {
    // if there is a connection waiting for the packet
    for(auto& conn : syn_queue_)
    {
      if(conn->remote() == packet->source())
      {
        conn->segment_arrived(packet);
        return;
      }
    }
  }
  // if it's a new attempt (SYN)
  else
  {
    // if we don't like this client, do nothing
    if(! on_accept_(packet->source()) )
      return;

    // remove oldest connection if queue is full
    printf("<Listener::segment_arrived> SynQueue: %u\n", syn_queue_.size());
    if(syn_queue_full())
    {
      printf("<Listener::segment_arrived> Queue is full\n");
      Expects(not syn_queue_.empty());
      //printf("<Listener::segment_arrived> Connection %s dropped to make room for new connection\n",
      //  syn_queue_.back()->to_string().c_str());
      //syn_queue_.back()->clean_up();

      syn_queue_.pop_back();
    }
    printf("<Listener::segment_arrived> SynQueue: %u\n", syn_queue_.size());

    auto& conn = *(syn_queue_.emplace(syn_queue_.begin(),
      std::make_shared<Connection>( host_, port_, packet->source() )));
    // Call Listener::connected when Connection is connected
    conn->on_connect(ConnectCallback::from<Listener, &Listener::connected>(this));
    conn->_on_cleanup(CleanupCallback::from<Listener, &Listener::remove>(this));
    // Open connection
    conn->open(false);
    Ensures(conn->is_listening());
    printf("<Listener::segment_arrived> Connection %s created\n",
      conn->to_string().c_str());
    conn->segment_arrived(packet);
    return;
  }
}

void Listener::remove(Connection_ptr conn) {
  printf("<Listener::remove> Inside remove\n");
  auto it = syn_queue_.begin();
  while(it++ != syn_queue_.end())
  {
    if((*it) == conn)
    {
      syn_queue_.erase(it);
      printf("<Listener::remove> %s removed.\n", conn->to_string().c_str());
      return;
    }
  }
}

void Listener::connected(Connection_ptr conn) {
  printf("<Listener::connected> %s connected\n", conn->to_string().c_str());
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
