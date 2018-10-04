// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
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

#include <net/http/server.hpp>
#include <net/inet>
#include <smp>

namespace http {

  const Server::idle_duration Server::DEFAULT_IDLE_TIMEOUT{std::chrono::seconds(60)};

  Server::Server(TCP& tcp, Request_handler cb, idle_duration timeout)
    : tcp_(tcp),
      on_request_(std::move(cb)),
      keep_alive_(true),
      timer_id_(Timers::UNUSED_ID),
      idle_timeout_(timeout),
      stat_conns_{Statman::get().create(Stat::UINT32, tcp.stack().ifname() + ".http_server.connections")},
      stat_req_rx_{Statman::get().create(Stat::UINT64, tcp.stack().ifname() + ".http_server.requests_rx")},
      stat_req_bad_{Statman::get().create(Stat::UINT32, tcp.stack().ifname() + ".http_server.requests_bad")},
      stat_timeouts_{Statman::get().create(Stat::UINT32, tcp.stack().ifname() + ".http_server.timeouts")}
  {
  }

  void Server::listen(uint16_t port)
  {
    assert(on_request_ != nullptr && "You must set 'on_request' on the server to receive requests!");

    bind(port);

    using namespace std::chrono;

    if(idle_timeout_ != idle_duration::zero())
      timer_id_ = Timers::periodic(30s, 1min, {this, &Server::timeout_clients});
  }

  Response_ptr Server::create_response(status_t code) const
  {
    auto res = make_response();
    res->set_status_code(code);
    return res;
  }

  Server::~Server()
  {
    if(timer_id_ != Timers::UNUSED_ID)
    {
      Timers::stop(timer_id_);
    }
  }

  void Server::bind(const uint16_t port)
  {
    assert(tcp_.get_cpuid() == SMP::cpu_id());
    tcp_.listen(port, {this, &Server::on_connect});
    SMP::global_lock();
    INFO("HTTP Server", "Listening on port %u on CPU %d", port, SMP::cpu_id());
    SMP::global_unlock();
  }

  void Server::connect(Connection::Stream_ptr stream)
  {
    debug("Connection attempt from %s\n", stream->remote().to_string().c_str());
    // if there is a free spot in connections
    if(free_idx_.size() > 0) {
      auto idx = free_idx_.back();
      Ensures(connections_[idx] == nullptr);
      connections_[idx] = std::make_unique<Server_connection>(*this, std::move(stream), idx);
      free_idx_.pop_back();
    }
    // if not, add a new shared ptr
    else {
      connections_.emplace_back(std::make_unique<Server_connection>(*this, std::move(stream), connections_.size()));
    }
    ++stat_conns_;
  }

  void Server::close(Server_connection& conn)
  {
    const auto idx = conn.idx();
    connections_[idx] = nullptr;
    free_idx_.push_back(idx);
  }

  void Server::timeout_clients(int32_t)
  {
    const auto count = idle_timeout_.count();
    for(auto& conn : connections_) {
      if(conn != nullptr and RTC::now() > (conn->idle_since() + count))
      {
        conn->timeout();
        ++stat_timeouts_;
      }
    }
  }

  void Server::receive(Request_ptr req, status_t code, Server_connection& conn)
  {
    ++stat_req_rx_;
    if(code == OK)
    {
      on_request_(std::move(req), std::make_unique<Response_writer>( create_response(code), conn ));
    }
    else
    {
      ++stat_req_bad_;
      // an error occured when parsing
      // call user on_error or something
      conn.send(create_response(code));
    }
  }

}
