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

#ifndef SERVER_HPP
#define SERVER_HPP

#include <net/inet4>
#include <net/dhcp/dh4client.hpp>

#include "middleware.hpp"
#include "request.hpp"
#include "response.hpp"
#include "connection.hpp"
#include "router.hpp"

namespace server {

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
  using Port     = const unsigned;
  using IP_Stack = std::shared_ptr<net::Inet4<VirtioNet>>;
  using OnConnect = net::tcp::Connection::ConnectCallback;
  using Path = std::string;
  struct MappedCallback {
    Path path;
    Callback callback;
    MappedCallback(Path pth, Callback cb) : path(pth), callback(cb) {}
  };
  using MiddlewareStack = std::vector<MappedCallback>;
  //-------------------------------
public:
  //-------------------------------
  // Default constructor to set up
  // the server
  //-------------------------------
  explicit Server();

  Server(IP_Stack);

  //-------------------------------
  // Default destructor
  //-------------------------------
  ~Server() noexcept = default;

  net::Inet4<VirtioNet>& ip_stack() const;

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
  void listen(Port port);

  void close(size_t conn_idx);

  void process(Request_ptr, Response_ptr);

  inline void use(Middleware_ptr mw)
  { use("/", mw); }

  void use(const Path&, Middleware_ptr);

  inline void use(Callback cb)
  { use("/", cb); }

  void use(const Path&, Callback);

  size_t active_clients() const
  { return connections_.size() - free_idx_.size(); }

private:
  //-------------------------------
  // Class data members
  //-------------------------------
  IP_Stack inet_;
  Router   router_;
  std::vector<Connection_ptr> connections_;
  std::vector<size_t> free_idx_;
  MiddlewareStack middleware_;
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

  //-------------------------------
  // Set up the network stack
  //-------------------------------
  void initialize();

  void connect(net::tcp::Connection_ptr);

  void process_route(Request_ptr, Response_ptr);

  Next create_next(std::shared_ptr<MiddlewareStack::iterator>, Request_ptr, Response_ptr);

}; //< class Server

template <typename Route_Table>
inline Server& Server::set_routes(Route_Table&& routes) {
  router_.install_new_configuration(std::forward<Route_Table>(routes));
return *this;
}

} // namespace server

#endif //< SERVER_HPP
