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

#include "request.hpp"
#include "response.hpp"
#include "connection.hpp"
#include "router.hpp"

namespace server {

using Next = delegate<void()>;
using Middleware = delegate<void(Request_ptr, Response_ptr, Next)>;

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

  void close(Connection*);

  void process(Request_ptr, Response_ptr);

private:
  //-------------------------------
  // Class data members
  //-------------------------------
  IP_Stack inet_;
  Router   router_;
  std::list<Connection_ptr> connections_;

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

}; //< class Server

template <typename Route_Table>
inline Server& Server::set_routes(Route_Table&& routes) {
  router_.install_new_configuration(std::forward<Route_Table>(routes));
return *this;
}

} // namespace server

#endif //< SERVER_HPP
