#pragma once

#include <net/tcp/connection.hpp>
#include "common.hpp"
#include "readq.hpp"
#include <list>

class IrcServer;

class Server
{
public:
  using Connection = net::tcp::Connection_ptr;
  Server(sindex_t, IrcServer&);
  
  // incoming
  void connect(Connection conn);
  // outgoing
  void connect(Connection conn, std::string name, std::string pass);
  
  bool is_alive() const noexcept {
    return regis != 0;
  }
  bool is_regged() const noexcept {
    return regis == 7;
  }
  sindex_t get_id() const noexcept {
    return this->self;
  }
  // fast b64 assuming max 26 servers
  char     token() const noexcept {
    return 'A' + token_;
  }
  uint8_t server_id() const noexcept {
    return token_;
  }
  uint8_t near_link() const noexcept {
    return near_link_;
  }
  bool is_local() const noexcept {
    // alt: near_link == server.server_id()
    return conn != nullptr;
  }

  const std::string& name() const noexcept {
    return sname;
  }
  const std::string& description() const noexcept {
    return sdesc;
  }
  const std::string& get_pass() const noexcept {
    return spass;
  }
  
  IrcServer& get_server() noexcept {
    return server;
  }
  
  void send(const std::string&);
  void send(const char*, size_t);
  
  void split_message(const std::string&);
  
  void squit(const std::string& reason);
  
  const std::string& name_hash() const noexcept {
    return sname;
  }
private:
  void setup_dg();
  void handle_commands(const std::vector<std::string>&);
  void handle_unknown(const std::vector<std::string>&);
  void try_auth();

  sindex_t self;
  uint8_t  regis;
  uint8_t  token_; // this servers ID
  uint8_t  near_link_;  // server ID towards us

  IrcServer&  server;
  Connection  conn;

  std::string sname;
  std::string spass;
  std::string sdesc;
  
  std::list<sindex_t> remote_links;

  ReadQ readq;
};
