#pragma once

#include <net/tcp/connection.hpp>
#include <cstdint>
#include <string>
#include <list>
#include "common.hpp"
#include "modes.hpp"
#include "readq.hpp"

#define WARNED_BIT    128

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
    return (regis & 7) != 0;
  }
  bool is_reg() const noexcept
  {
    return (regis & 7) == 7;
  }
  bool is_local() const noexcept
  {
    return remote_id == NO_SUCH_CLIENT;
  }
  uint8_t get_server_id() const noexcept {
    return server_id;
  }
  // reset to a new connection
  void reset_to(Connection conn);
  void reset_to(clindex_t uid, sindex_t sid, clindex_t rid,
      const std::string& n, const std::string& u, const std::string& h, const std::string& rn);
  // disable client completely
  void disable();

  clindex_t get_id() const noexcept {
    return self;
  }
  IrcServer& get_server() const noexcept {
    return server;
  }
  clindex_t get_token_id() const noexcept {
    if (is_local())
        return this->self;
    else
        return this->remote_id;
  }
  std::string token() const;

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
  void send_buffer(net::tcp::buffer_t buff);

  const std::string& nick() const noexcept {
    return nick_;
  }
  const std::string& user() const noexcept {
    return user_;
  }
  const std::string& host() const noexcept {
    return host_;
  }
  const std::string& ip_addr() const noexcept {
    return ip_;
  }
  const std::string& realname() const noexcept {
    return rname_;
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
  void propagate_quit(const char*, int len);

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
    if (warned) regis |= WARNED_BIT;
    else        regis &= ~WARNED_BIT;
  }
  bool is_warned() const noexcept {
    return regis & WARNED_BIT;
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

  static void init();
private:
  void split_message(const std::string&);
  void handle_new(const std::vector<std::string>&);
  void handle_cmd(const std::vector<std::string>&);

  clindex_t   self;
  clindex_t   remote_id;
  uint8_t     regis;
  uint8_t     server_id;
  uint16_t    umodes_;

  IrcServer&  server;
  Connection  conn;
  long        to_stamp;

  std::string nick_;
  std::string user_;
  std::string host_;
  std::string ip_;
  std::string rname_;
  ChannelList channels_;

  ReadQ readq;
};
