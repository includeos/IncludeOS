#pragma once

#include <net/tcp/connection.hpp>
#include <cstdint>
#include <string>
#include <list>
#include "common.hpp"
#include "modes.hpp"

#define WARNED_BIT    1

class  IrcServer;
namespace liu {
  struct Storage;
  struct Restore;
}

class Client
{
public:
  using Connection = net::tcp::Connection_ptr;
  using ChannelList = std::list<chindex_t>;
  
  Client(clindex_t s, IrcServer& sref);
  
  bool is_alive() const noexcept
  {
    return regis != 0;
  }
  bool is_reg() const noexcept
  {
    return regis == 7;
  }
  bool is_local() const noexcept
  {
    return conn != nullptr;
  }
  // reset to a new connection
  void reset_to(Connection conn);
  // disable client completely
  void disable();

  clindex_t get_id() const noexcept {
    return self;
  }
  IrcServer& get_server() const noexcept {
    return server;
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
  
  const std::string& nick() const noexcept {
    return nick_;
  }
  const std::string& user() const noexcept {
    return user_;
  }
  const std::string& host() const noexcept {
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
  bool on_channel(chindex_t) const noexcept;
  
  // close connection with given reason
  void kill(bool warn, const std::string&);
  // tell everyone this client has quit
  void handle_quit(const char*, int len);
  
  long get_timeout_ts() const noexcept {
    return to_stamp;
  }
  
  void set_vhost(const std::string& new_vhost)
  {
    this->host_ = new_vhost;
  }
  
  void set_to_stamp(long new_tos) noexcept {
    to_stamp = new_tos;
  }
  void set_warned(bool warned) noexcept {
    if (warned) bits |= WARNED_BIT;
    else        bits &= ~WARNED_BIT;
  }
  bool is_warned() const noexcept {
    return bits & WARNED_BIT;
  }
  
  size_t club() const noexcept {
    return nick_.size() + user_.size() + host_.size() + readq.size() + sizeof(conn) + sizeof(*conn);
  }
  
  Connection& get_conn() noexcept {
    return conn;
  }
  void assign_socket_dg();
  void serialize_to(liu::Storage&);
  void deserialize(liu::Restore&);

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
  
  const std::string& name_hash() const noexcept {
    return nick_;
  }
private:
  void split_message(const std::string&);
  void handle_new(const std::vector<std::string>&);
  void handle_cmd(const std::vector<std::string>&);
  
  clindex_t   self;
  uint8_t     regis;
  uint8_t     bits;
  uint16_t    umodes_;
  IrcServer&  server;
  Connection  conn;
  long        to_stamp;
  
  std::string nick_;
  std::string user_;
  std::string host_;
  ChannelList channels_;
  
  std::string readq;
};
