#pragma once

#include "common.hpp"
#include <net/tcp/connection.hpp>

class IrcServer;

class Server
{
public:
  using Connection = net::tcp::Connection_ptr;
  Server(sindex_t, IrcServer&);
  
  // incoming
  void connect(Connection conn);
  // outgoing
  void connect(sindex_t, IrcServer&, net::IP4, uint16_t, std::string);
  
  bool is_alive() const noexcept {
    return regis != 0;
  }
  bool is_regged() const noexcept {
    return regis == 7;
  }

  const std::string& name() const noexcept {
    return sname;
  }
  const std::string& description() const noexcept {
    return sdesc;
  }
  
  IrcServer& get_server() noexcept {
    return server;
  }
  
  void squit(const std::string& reason);
  
  const std::string& name_hash() const noexcept {
    return sname;
  }
private:
  void recv(const std::string&);
  void handle_commands(const std::vector<std::string>&);
  void handle_unknown(const std::vector<std::string>&);
  void try_auth();

  sindex_t self;
  uint8_t  regis;

  IrcServer&  server;
  Connection  conn;

  std::string sname;
  std::string spass;
  std::string sdesc;

  std::string readq;
};
