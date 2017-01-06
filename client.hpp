#pragma once

#include <net/inet4>
#include <cstdint>
#include <string>
#include <list>
#include "common.hpp"
#include "modes.hpp"

#define WARNED_BIT    1

class IrcServer;

class Client
{
public:
  using Connection = net::tcp::Connection_ptr;
  using ChannelList = std::list<chindex_t>;
  
  Client(clindex_t s, IrcServer& sref);
  
  bool is_alive() const
  {
    return regis != 0;
  }
  bool is_reg() const
  {
    return regis == 7;
  }
  bool is_local() const
  {
    return conn != nullptr;
  }
  // reset to a new connection
  void reset_to(Connection conn);
  // disable client completely
  void disable();
  
  clindex_t get_id() const {
    return self;
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
  
  void read(uint8_t* buffer, size_t len);
  void send_from(const std::string& from, const std::string& text);
  void send_from(const std::string& from, uint16_t numeric, const std::string& text);
  void send(uint16_t numeric, std::string text);
  // send the string as-is
  void send_raw(const char* buff, size_t len);
  void send_buffer(net::tcp::buffer_t buff, size_t len);
  
  const std::string& nick() const {
    return nick_;
  }
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
    std::string temp;
    temp.reserve(64);
    temp += user_;
    temp += "@";
    temp += host_;
    return temp;
  }
  std::string nickuserhost() const
  {
    std::string temp;
    temp.reserve(64);
    
    temp += nick_;
    temp += "!";
    temp += user_;
    temp += "@";
    temp += host_;
    return temp;
  }
  
  ChannelList& channels() {
    return channels_;
  }
  
  // close connection with given reason
  void kill(bool warn, const std::string&);
  // tell everyone this client has quit
  void handle_quit(const char*, int len);
  
  long get_timeout_ts() const {
    return to_stamp;
  }
  
  void set_vhost(const std::string& new_vhost)
  {
    this->host_ = new_vhost;
  }
  
  void set_to_stamp(long new_tos) {
    to_stamp = new_tos;
  }
  void set_warned(bool warned) {
    if (warned) bits |= WARNED_BIT;
    else        bits &= ~WARNED_BIT;
  }
  bool is_warned() const {
    return bits & WARNED_BIT;
  }
  
  size_t club() const {
    return nick_.size() + user_.size() + host_.size() + readq.size() + sizeof(conn) + sizeof(*conn);
  }
  
  Connection& getconn() { return conn; }
  
private:
  void split_message(const std::string&);
  void handle_new(const std::string&, const std::vector<std::string>&);
  void handle(const std::string&, const std::vector<std::string>&);
  
  void welcome(uint8_t);
  void auth_notice();
  void send_motd();
  void send_lusers();
  void send_modes();
  void send_stats(const std::string&);
  void send_quit(const std::string& reason);
  bool change_nick(const std::string& new_nick);
  
  void not_ircop(const std::string& cmd);
  void need_parms(const std::string& cmd);
  
  uint8_t     regis;
  uint8_t     bits;
  uint16_t    umodes_;
  clindex_t   self;
  IrcServer&  server;
  Connection  conn;
  long        to_stamp;
  
  std::string nick_;
  std::string user_;
  std::string host_;
  ChannelList channels_;
  
  std::string readq;
};
