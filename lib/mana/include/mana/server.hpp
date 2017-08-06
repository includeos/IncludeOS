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

#ifndef MANA_SERVER_HPP
#define MANA_SERVER_HPP

#include <net/http/server.hpp>

#include "middleware.hpp"
#include "request.hpp"
#include "response.hpp"
#include "router.hpp"

namespace mana {

inline bool path_starts_with(const std::string& path, const std::string& start) {
  if(path.size() < start.size())
    return false;
  if(path == start)
    return true;
  return path.substr(0, std::min(start.size(), path.size())) == start;
}

//-------------------------------
// This class is a simple dumb
// HTTP server for service testing
//-------------------------------
class Server {
private:
  //-------------------------------
  // Internal class type aliases
  //-------------------------------
  using Port      = const unsigned;
  using Path      = std::string;
  struct MappedCallback {
    Path path;
    Callback callback;
    MappedCallback(Path pth, Callback cb) : path(pth), callback(cb) {}
  };
  using MiddlewareStack = std::vector<MappedCallback>;
  //-------------------------------
public:

  explicit Server(net::TCP&, std::chrono::seconds timeout = http::Server::DEFAULT_IDLE_TIMEOUT);

  //-------------------------------
  // Default destructor
  //-------------------------------
  ~Server() noexcept = default;

  //-------------------------------
  // Get the underlying router
  // which contain route resolution
  // configuration
  //-------------------------------
  Router& router() noexcept;

  //-------------------------------
  // Install a new route table for
  // route resolutions
  //
  // @tparam (http::Router) new_routes - The new route table
  //                                     to install
  //
  // @return - The object that invoked this method
  //-------------------------------
  template <typename Route_Table>
  Server& set_routes(Route_Table&& routes);

  //-------------------------------
  // Start the server to listen for
  // incoming connections on the
  // specified port
  //-------------------------------
  void listen(Port port)
  { server_.listen(port); }

  void use(const Path&, Middleware_ptr);

  void use(Middleware_ptr mw)
  { use("/", std::move(mw)); }

  void use(const Path&, Callback);

  void use(Callback cb)
  { use("/", std::move(cb)); }

  http::Server& http_server() noexcept
  { return server_; }

  const http::Server& http_server() const noexcept
  { return server_; }

  size_t connected_clients() const noexcept
  { return server_.connected_clients(); }

private:
  //-------------------------------
  // Class data members
  //-------------------------------
  http::Server                server_;
  Router                      router_;
  MiddlewareStack             middleware_;
  std::vector<Middleware_ptr> mw_storage_;

  //-----------------------------------
  // Deleted move and copy operations
  //-----------------------------------
  Server(const Server&) = delete;
  Server(Server&&) = delete;

  //-----------------------------------
  // Deleted move and copy assignment operations
  //-----------------------------------
  Server& operator = (const Server&) = delete;
  Server& operator = (Server&&) = delete;

  void handle_request(http::Request_ptr, http::Response_writer_ptr);

  void process(Request_ptr req, Response_ptr res);

  void process_route(Request_ptr, Response_ptr);

}; //< class Server

template <typename Route_Table>
inline Server& Server::set_routes(Route_Table&& routes) {
  router_.install_new_configuration(std::forward<Route_Table>(routes));
  return *this;
}

} // namespace mana

#endif //< MANA_SERVER_HPP
