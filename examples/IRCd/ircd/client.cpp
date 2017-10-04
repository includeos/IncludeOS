#include "client.hpp"

#include "ircsplit.hpp"
#include "ircd.hpp"
#include "tokens.hpp"
#include <common>
#include <cassert>
#include "yield_assist.hpp"
#include <profile>
#include <algorithm>

Client::Client(clindex_t s, IrcServer& sref)
  : self(s), remote_id(NO_SUCH_CLIENT), regis(0), server(sref), conn(nullptr) {}

std::string Client::token() const
{
  static const char* base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

  std::string tk; tk.resize(4);
  if (is_local())
      tk[0] = server.token();
  else
      tk[0] = server.servers.get(server_id).token();

  clindex_t idx = get_token_id();
  for (int i = 0; i < 18; i += 6) {
    tk[i+1] = base64_chars[(idx >> i) & 63];
  }
  return tk;
}

void Client::reset_to(Connection conn)
{
  // this resets the client to a new connection
  // regis field is 1, which means there is a connection
  this->regis     = 1;
  this->server_id = server.server_id();
  this->umodes_   = default_user_modes();
  this->remote_id = NO_SUCH_CLIENT;
  this->conn      = conn;
  this->readq.clear();
  // assign correct delegates
  this->assign_socket_dg();

  // send auth notices
  auth_notice();
}
void Client::reset_to(clindex_t uid, sindex_t sid, clindex_t rid,
      const std::string& nick, const std::string& user, const std::string& host, const std::string& rname)
{
  this->self    = uid;
  this->remote_id = rid;
  this->regis   = 7; // ?
  this->server_id = sid;
  this->umodes_ = 0;
  this->conn    = nullptr;
  this->nick_   = nick;
  this->user_   = user;
  this->host_   = host;
  this->rname_  = rname;
}
void Client::assign_socket_dg()
{
  // set up callbacks
  conn->on_read(128, {this, &Client::read});

  conn->on_close(
  [this] {
    //assert(this->is_alive());
    if (UNLIKELY(!this->is_alive())) return;
    // tell everyone that he just disconnected
    char buff[128];
    int len = snprintf(buff, sizeof(buff),
              ":%s QUIT :%s\r\n", this->nickuserhost().c_str(), "Connection closed");
    this->propagate_quit(buff, len);
    // force-free resources
    this->disable();
  });
}

void Client::disable()
{
  conn = nullptr;
  // free client on server
  server.free_client(*this);
  // reset registration status
  regis = 0;
  // stop timeout timer
  this->to_timer.stop();
  // free memory properly
  this->nick_.clear();
  this->nick_.shrink_to_fit();
  this->user_.clear();
  this->user_.shrink_to_fit();
  this->host_.clear();
  this->host_.shrink_to_fit();
  this->channels_.clear();
  this->readq.clear();
}

#include <kernel/syscalls.hpp>
void Client::split_message(const std::string& msg)
{
  // in case splitter is bad
  SET_CRASH_CONTEXT("Client::split_message():\n'%.*s'", (int) msg.size(), msg.c_str());

  auto vec = ircsplit(msg);

  // ignore empty messages
  if (UNLIKELY(vec.empty())) return;
  // transform command to uppercase
  extern void transform_to_upper(std::string&);
  transform_to_upper(vec[0]);

//#define PRINT_CLIENT_MESSAGE
#ifdef PRINT_CLIENT_MESSAGE
  printf("[%u]: ", self);
  for (auto& str : vec)
  {
    printf("[%s]", str.c_str());
  }
  printf("\n");
#endif

  // reset timeout now that we got data
  this->set_warned(false);
  this->restart_timeout();

  if (this->is_reg())
      handle_cmd(vec);
  else
      handle_new(vec);
}

void Client::read(net::tcp::buffer_t buffer)
{
  if (readq.read(buffer->data(), buffer->size(),
      {this, &Client::split_message}) == false)
  {
    kill(false, "Max readq exceeded");
  }
}

void Client::send_from(const std::string& from, const std::string& text)
{
  char data[128];
  int len = snprintf(data, sizeof(data),
    ":%s %s\r\n", from.c_str(), text.c_str());

  send_raw(data, len);
}
void Client::send_from(const std::string& from, uint16_t numeric, const std::string& text)
{
  char data[128];
  int len = snprintf(data, sizeof(data),
    ":%s %03u %s\r\n", from.c_str(), numeric, text.c_str());

  send_raw(data, len);
}
void Client::send(uint16_t numeric, std::string text)
{
  char data[128];
  int len = snprintf(data, sizeof(data),
    ":%s %03u %s %s\r\n", server.name().c_str(), numeric, nick().c_str(), text.c_str());

  send_raw(data, len);
}
void Client::send_raw(const char* buff, size_t len)
{
  if (!conn->is_connected()) {
    //printf("!! Skipped dead connection: %s\n", conn->state().to_string().c_str());
    return;
  }
  //static YieldCounter counter(100);
  conn->write(buff, len);
  //++counter;
}
void Client::send_buffer(net::tcp::buffer_t buff)
{
  if (!conn->is_connected()) {
    //printf("!! Skipped dead connection: %s\n", conn->state().to_string().c_str());
    return;
  }
  conn->write(buff);
}


// validate name, returns false if invalid characters
static bool validate_name(const std::string& new_name)
{
  // empty nickname is invalid
  if (new_name.empty()) return false;
  // forbidden first characters
  if (isdigit(new_name[0])) return false;
  // a-z A-Z 0-9 _ - \ [ ] { } ^ ` |
  static const std::string LUT =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890_-\\[]{}^`|ÆØÅæøå";
  // forbidden characters
  if (LUT.find_first_of(new_name) == std::string::npos) return false;
  return true;
}
bool Client::change_nick(const std::string& new_nick)
{
  if (new_nick.size() < server.nick_minlen()) {
    if (nick_.empty())
        send_from(server.name(), ERR_ERRONEUSNICKNAME, new_nick + " " + new_nick + " :Nickname too short");
    else
        send_from(server.name(), ERR_ERRONEUSNICKNAME, nick() + " " + new_nick + " :Nickname too short");
    return false;
  }
  if (new_nick.size() > server.nick_maxlen()) {
    if (nick_.empty())
        send_from(server.name(), ERR_ERRONEUSNICKNAME, new_nick + " " + new_nick + " :Nickname too long");
    else
        send_from(server.name(), ERR_ERRONEUSNICKNAME, nick() + " " + new_nick + " :Nickname too long");
    return false;
  }
  if (!validate_name(new_nick)) {
    if (nick_.empty())
        send_from(server.name(), ERR_ERRONEUSNICKNAME, new_nick + " " + new_nick + " :Erroneous nickname");
    else
        send_from(server.name(), ERR_ERRONEUSNICKNAME, nick() + " " + new_nick + " :Erroneous nickname");
    return false;
  }
  auto idx = server.clients.find(new_nick);
  if (idx != NO_SUCH_CLIENT) {
    if (nick_.empty())
        send_from(server.name(), ERR_NICKNAMEINUSE, new_nick + " " + new_nick + " :Nickname is already in use");
    else
        send_from(server.name(), ERR_NICKNAMEINUSE, nick() + " " + new_nick + " :Nickname is already in use");
    return false;
  }
  // remove old nickname from hashtable
  if (!nick().empty())
      server.clients.erase_hash(nick());
  // store new nickname
  server.clients.hash(new_nick, get_id());
  // nickname is valid and free, take it
  this->nick_ = new_nick;
  return true;
}

std::string Client::mode_string() const
{
  std::string res;
  res.reserve(4);

  for (int i = 0; i < 8; i++)
  {
    if (umodes_ & (1 << i))
        res += usermodes.bit_to_char(i);
  }
  return res;
}

bool Client::on_channel(chindex_t idx) const noexcept
{
  return std::find(channels_.begin(), channels_.end(), idx) != channels_.end();
}

void Client::kill(bool warn, const std::string& reason)
{
  char buff[256];
  int len = snprintf(buff, sizeof(buff),
      ":%s QUIT :%s\r\n", nickuserhost().c_str(), reason.c_str());

  // inform everyone what happened
  if (is_reg())
      propagate_quit(buff, len);
  // ignore socketry for remote clients
  if (is_local() == false) return;
  // inform neighbors about local client quitting
  server.sbcast(token() + " Q :" + reason);

  if (warn) {
    // close connection after write
    conn->write(buff, len);
  }
  conn->close();
}

void Client::propagate_quit(const char* buff, int len)
{
  // inform others about disconnect
  server.user_bcast_butone(get_id(), buff, len);
  // remove client from various lists
  for (size_t idx : channels())
  {
    Channel& ch = server.channels.get(idx);
    ch.remove(get_id());

    // if the channel became empty, remove it
    if (ch.is_alive() == false)
        server.free_channel(ch);
  }
}
