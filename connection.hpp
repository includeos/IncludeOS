#ifndef SERVER_CONNECTION_HPP
#define SERVER_CONNECTION_HPP

#include "net/tcp.hpp"
#include "request.hpp"
#include "response.hpp"

namespace server {

class Server;

class Connection;
using Connection_ptr = std::shared_ptr<Connection>;


class Connection {
private:
  const static size_t BUFSIZE = 1460;
  using Connection_ptr = net::TCP::Connection_ptr;
  using buffer_t = net::TCP::buffer_t;
  using OnData = net::TCP::Connection::ReadCallback;
  using Disconnect = net::TCP::Connection::Disconnect;
  using OnDisconnect = net::TCP::Connection::DisconnectCallback;

public:
  Connection(Server&, Connection_ptr);

  Request_ptr get_request()
  { return request_; }

  Response_ptr get_response()
  { return response_; }


  void close();


private:
  Server& server_;
  Connection_ptr conn_;
  Request_ptr request_;
  Response_ptr response_;

  void on_data(buffer_t, size_t);

  void on_disconnect(Connection_ptr, Disconnect);

}; // < server::Connection

}; // < server

#endif
