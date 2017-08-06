// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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
#include <statman>

// #define DEBUG

using namespace mana;
using namespace std::chrono;

Server::Server(net::TCP& tcp, std::chrono::seconds timeout)
  : server_{tcp, {this, &Server::handle_request}, timeout}
{
}

Router& Server::router() noexcept {
  return router_;
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

void Server::handle_request(http::Request_ptr request, http::Response_writer_ptr hrw)
{
  auto req = std::make_shared<mana::Request>(std::move(request));
  auto res = std::make_shared<mana::Response>(std::move(hrw));
  process(std::move(req), std::move(res));
}
