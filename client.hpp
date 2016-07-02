#pragma once

#include <net/inet4>
#include <cstdint>
#include <string>
#include <list>
#include "modes.hpp"

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
  bool is_local() const
  {
    return conn != nullptr;
  }
  void disable();
  void reset(Connection conn);
  
  index_t get_id() const {
    return self;
  }
  const std::string& nick() const {
    return nick_;
  }
  
  bool is_operator() const {
    return this->umodes_ & usermodes.char_to_bit(UMODE_IRCOP);
  }
  void add_umodes(uint16_t mask) {
    this->umodes_ |= mask;
  }
  void rem_umodes(uint16_t mask) {
    this->umodes_ &= ~mask;
  }
  
  void read(const uint8_t* buffer, size_t len);
  void send_from(const std::string& from, uint16_t numeric, const std::string& text);
  void send_nonick(uint16_t numeric, const std::string& text);
  void send(uint16_t numeric, std::string text)
  {
    send_nonick(numeric, nick() + " " + text);
  }
  // send as server to client
  void send(std::string text);
  // send the string as-is
  void send_raw(std::string text);
  
  const std::string& user() const
  {
    return user_;
  }
  const std::string& host() const
  {
    return host_;
  }
  
  std::string mode_string() const;
  
  std::string userhost() const
  {
    return user_ + "@" + host_;
  }
  std::string nickuserhost() const
  {
    return nick_ + "!" + userhost();
  }
  
  ChannelList& channels() {
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
  void send_modes();
  bool change_nick(const std::string& new_nick);
  
  bool        alive_;
  uint8_t     regis;
  uint16_t    umodes_;
  index_t     self;
  IrcServer&  server;
  Connection  conn;
  
  std::string nick_;
  std::string user_;
  std::string host_;
  ChannelList channels_;
  
  std::string buffer;
};
