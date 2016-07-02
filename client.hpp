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
  using ChannelList = std::list<uint16_t>;
  typedef uint16_t index_t;
  
  Client(size_t s, IrcServer& sref)
    : alive_(true), regis(0), self(s), server(sref) {}
  
  bool alive() const
  {
    return alive_;
  }
  bool is_reg() const
  {
    return alive() && regis == 3;
  }
  void disable()
  {
    alive_ = false; regis = 0;
  }
  void reset(Connection conn);
  
  index_t get_id() const {
    return self;
  }
  const std::string& nick() const {
    return nick_;
  }
  
  bool is_operator() const {
    return this->umodes & UMODE_IRCOP_MASK;
  }
  void set_umodes(uint16_t mask) {
    this->umodes |= mask;
  }
  
  void read(const uint8_t* buffer, size_t len);
  void send_nonick(uint16_t numeric, std::string text);
  void send(uint16_t numeric, std::string text);
  void send(std::string text);
  
  const std::string& user() const
  {
    return user_;
  }
  const std::string& host() const
  {
    return host_;
  }
  
  std::string userhost() const
  {
    return user_ + "@" + host_;
  }
  std::string nickuserhost() const
  {
    return nick_ + "!" + userhost();
  }
  
  ChannelList& chans() {
    return channels_;
  }
  
private:
  void split_message(const std::string&);
  void handle_new(const std::string&, const std::vector<std::string>&);
  void handle(const std::string&, const std::vector<std::string>&);
  
  void welcome(uint8_t);
  void auth_notice();
  void send_motd();
  void send_lusers();
  bool change_nick(const std::string& new_nick);
  
  bool        alive_;
  uint8_t     regis;
  uint16_t    umodes;
  index_t     self;
  IrcServer&  server;
  Connection  conn;
  
  std::string nick_;
  std::string user_;
  std::string host_;
  ChannelList channels_;
  
  std::string buffer;
};
