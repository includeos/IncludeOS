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

#include "../include/mana/server.hpp"
#include <utility>
#include <timers>
#include <statman>

// #define DEBUG

using namespace mana;
using namespace std::chrono;

Server::Server(IP_stack& stack)
: inet_(stack)
{
  setup_stats();
}

void Server::setup_stats() {
  auto& statman = Statman::get();
  auto& total_conn  = statman.create(Stat::UINT64, "http.connection_total");
  auto& total_res   = statman.create(Stat::UINT64, "http.responses");
  auto& bytes_sent  = statman.create(Stat::UINT64, "http.bytes_sent");
  auto& total_req   = statman.create(Stat::UINT64, "http.requests");
  auto& bytes_recv  = statman.create(Stat::UINT64, "http.bytes_recv");

  Connection::on_connection(
  [&total_conn] ()
  {
    ++total_conn;
  });

  Response::on_sent(
  [&total_res, &bytes_sent] (size_t n)
  {
    ++total_res;
    bytes_sent.get_uint64() += n;
  });

  Request::on_recv(
  [&total_req, &bytes_recv] (size_t n)
  {
    ++total_req;
    bytes_recv.get_uint64() += n;
  });
}

Router& Server::router() noexcept {
  return router_;
}

void Server::listen(Port port) {
  printf("<Server> Listening to port %i \n", port);

  inet_.tcp().bind(port).on_connect({this, &Server::connect});
  Timers::periodic(30s, 1min, {this, &Server::timeout_clients});
}

void Server::connect(net::tcp::Connection_ptr conn) {
  SET_CRASH_CONTEXT("Server::connect: %s, free_idx=%u",
    conn->to_string().c_str(), free_idx_.size());
  #ifdef VERBOSE_WEBSERVER
  printf("<Server> New Connection [ %s ]\n", conn->remote().to_string().c_str());
  #endif
  // if there is a free spot in connections
  if(free_idx_.size() > 0) {
    auto idx = free_idx_.back();
    Ensures(connections_[idx] == nullptr);
    connections_[idx] = std::make_unique<Connection>(*this, conn, idx);
    free_idx_.pop_back();
  }
  // if not, add a new shared ptr
  else {
    connections_.emplace_back(std::make_unique<Connection>(*this, conn, connections_.size()));
  }
}

void Server::close(size_t idx) {
  connections_[idx] = nullptr;
  free_idx_.push_back(idx);
}

void Server::process(Request_ptr req, Response_ptr res) {
  auto it_ptr = std::make_shared<MiddlewareStack::iterator>(middleware_.begin());
  auto next = std::make_shared<next_t>();
  auto weak_next = std::weak_ptr<next_t>(next);
  // setup Next callback
  *next = next_t::make_packed(
  [this, it_ptr, weak_next, req, res]
  {
    // derefence the pointer to the iterator
    auto& it = *it_ptr;

    // skip those who don't match
    while(it != middleware_.end() and !path_starts_with(req->uri(), it->path))
      it++;

    // while there is more to do
    if(it != middleware_.end()) {
      // dereference the function
      auto& func = it->callback;
      // advance the iterator for the next next-call
      it++;
      auto next = weak_next.lock(); // this should be safe since we're inside next
      // execute the function
      func(req, res, next);
    }
    // no more middleware, proceed with route processing
    else {
      process_route(req, res);
    }
  });
  // get the party started..
  (*next)();
}

void Server::process_route(Request_ptr req, Response_ptr res) {
  try {
    auto parsed_route = router_.match(req->method(), req->uri());
    req->set_params(parsed_route.parsed_values);
    parsed_route.job(req, res);
  }
  catch (const Router_error& err) {
    printf("<Server> Router_error: %s - Responding with 404.\n", err.what());
    res->send_code(http::Not_Found, true);
  }
}

void Server::use(const Path& path, Middleware_ptr mw_ptr) {
  mw_storage_.push_back(mw_ptr);
  mw_ptr->on_mount(path);
  use(path, mw_ptr->handler());
}

void Server::use(const Path& path, Callback callback) {
  middleware_.emplace_back(path, callback);
}

std::vector<net::tcp::Connection_ptr> Server::active_tcp_connections() const {
  std::vector<net::tcp::Connection_ptr> conns;

  for (auto& conn : connections_) {
    if (conn != nullptr)
      conns.push_back(conn->tcp_conn());
  }

  return conns;
}

void Server::timeout_clients(int32_t) {
  for(auto& conn : connections_) {
    if(conn != nullptr and RTC::now() > (conn->idle_since() + IDLE_TIMEOUT))
      conn->timeout();
  }
}
