#pragma once

#include <net/inet4>
#include <cstdint>
#include <string>
#include <list>
#include "umodes.hpp"

class IrcServer;

class Client
{
public:
  using Connection = net::TCP::Connection_ptr;
  using ChannelList = std::list<size_t>;
  typedef uint16_t index_t;
  
  Client(size_t s, IrcServer& sref)
    : alive(true), regis(0), self(s), server(sref) {}
  
  bool is_alive() const
  {
    return alive;
  }
  bool is_reg() const
  {
    return regis == 3;
  }
  void disable()
  {
    alive = false; regis = 0;
  }
  index_t get_id() const {
    return self;
  }
  bool is_operator() const {
    return this->umodes & UMODE_IRCOP_MASK;
  }
  void set_umodes(uint16_t mask) {
    this->umodes |= mask;
  }
  
  void set_connection(Connection conn) {
    this->conn = conn;
  }
  
  void read(const uint8_t* buffer, size_t len);
  void send(uint16_t numeric, std::string text);
  void send(std::string text);
  
  std::string userhost() const
  {
    return user + "@" + host;
  }
  std::string nickuserhost() const
  {
    return nick + "!" + userhost();
  }
  
  ChannelList& channels() {
    return channel_list;
  }
  
private:
  void split_message(const std::string&);
  void handle_new(const std::string&, const std::vector<std::string>&);
  void handle(const std::string&, const std::vector<std::string>&);
  
  void welcome(uint8_t);
  void auth_notice();
  
  bool        alive;
  uint8_t     regis;
  uint16_t    umodes;
  index_t     self;
  IrcServer&  server;
  Connection  conn;
  
  std::string nick;
  std::string user;
  std::string host;
  ChannelList channel_list;
  
  std::string buffer;
};
