#pragma once

#include "common.hpp"
#include <net/tcp/connection.hpp>

class IrcServer;

class Server
{
public:
  using Connection = net::tcp::Connection_ptr;
  Server(sindex_t, IrcServer&, Connection);
  
  bool is_alive() const noexcept
  {
    return regis != 0;
  }


  const std::string& name() const noexcept {
    return sname;
  }
  
  IrcServer& get_server() noexcept {
    return server;
  }
  
private:
  void recv(const std::string&);

  sindex_t self;
  uint8_t  regis;

  IrcServer&  server;
  Connection  conn;
  std::string sname;
  std::string spass;

  std::string readq;
};
