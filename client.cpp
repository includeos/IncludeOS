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
  : self(s), regis(0), server(sref)
{
  //readq.reserve(IrcServer::readq_max());
}

void Client::reset_to(Connection conn)
{
  // this resets the client to a new connection
  // regis field is 1, which means there is a connection
  this->regis = 1;
  this->bits  = 0;
  this->umodes_ = default_user_modes();
  this->conn = conn;
  this->to_stamp = server.create_timestamp();
  this->readq.clear();
  // assign correct delegates
  this->assign_socket_dg();
  
  // send auth notices
  auth_notice();
}
void Client::assign_socket_dg()
{
  // set up callbacks
  conn->on_read(128, 
  [srv = &server, idx = self] (auto buffer, size_t len)
  {
    /// NOTE: the underlying array can move around, 
    /// so we have to retrieve the address each time
    auto& client = srv->get_client(idx);
    client.read(buffer.get(), len);
  });

  conn->on_close(
  [srv = &server, idx = self] {
    // for the case where the client has not voluntarily quit,
    auto& client = srv->get_client(idx);
    //assert(client.is_alive());
    if (UNLIKELY(!client.is_alive())) return;
    // tell everyone that he just disconnected
    char buff[128];
    int len = snprintf(buff, sizeof(buff),
              ":%s QUIT :%s\r\n", client.nickuserhost().c_str(), "Connection closed");
    client.handle_quit(buff, len);
    // force-free resources
    client.disable();
  });
}

void Client::disable()
{
  conn = nullptr;
  // free client on server
  server.free_client(*this);
  // reset registration status
  regis = 0;
  // free memory properly
  this->nick_.clear();
  this->nick_.shrink_to_fit();
  this->user_.clear();
  this->user_.shrink_to_fit();
  this->host_.clear();
  this->host_.shrink_to_fit();
  this->channels_.clear();
  this->readq.clear();
  this->readq.shrink_to_fit();
}

#include <kernel/syscalls.hpp>
void Client::split_message(const std::string& msg)
{
  volatile ScopedProfiler profile;
  // in case splitter is bad
  SET_CRASH_CONTEXT("Client::split_message():\n'%.*s'", msg.size(), msg.c_str());
  
  std::string source;
  auto vec = ircsplit(msg, source);
  
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
  set_warned(false);
  to_stamp = server.get_cheapstamp();
  
  if (this->is_reg() == false)
    handle_new(source, vec);
  else
    handle(source, vec);
}

void Client::read(uint8_t* buf, size_t len)
{
  volatile ScopedProfiler profile;
  while (len > 0) {
    
    int search = -1;
    
    // find line ending
    for (size_t i = 0; i < len; i++)
    if (buf[i] == 13 || buf[i] == 10) {
      search = i; break;
    }
    
    // not found:
    if (UNLIKELY(search == -1))
    {
      // if clients are sending too much data to server, kill them
      if (UNLIKELY(readq.size() + len >= server.readq_max())) {
        kill(false, "Max readq exceeded");
        return;
      }
      // append entire buffer
      readq.append((const char*) buf, len);
      return;
    }
    else if (UNLIKELY(search == 0)) {
      buf++; len--;
    } else {
      
      // found CR LF:
      // if clients are sending too much data to server, kill them
      if (UNLIKELY(readq.size() + search >= server.readq_max())) {
        kill(false, "Max readq exceeded");
        return;
      }
      // append to clients buffer
      readq.append((const char*) buf, search);
      
      // move forward in socket buffer
      buf += search;
      // decrease len
      len -= search;
      
      // parse message
      if (readq.size())
      {
        split_message(readq);
        readq.clear();
      }
      
      // skip over continous line ending characters
      if (len != 0 && (buf[0] == 13 || buf[0] == 10)) {
          buf++; len--;
      }
    }
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
void Client::send_buffer(net::tcp::buffer_t buff, size_t len)
{
  if (!conn->is_connected()) {
    //printf("!! Skipped dead connection: %s\n", conn->state().to_string().c_str());
    return;
  }
  conn->write(buff, len);
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
  auto idx = server.user_by_name(new_nick);
  if (idx != NO_SUCH_CLIENT) {
    if (nick_.empty())
        send_from(server.name(), ERR_NICKNAMEINUSE, new_nick + " " + new_nick + " :Nickname is already in use");
    else
        send_from(server.name(), ERR_NICKNAMEINUSE, nick() + " " + new_nick + " :Nickname is already in use");
    return false;
  }
  // remove old nickname from hashtable
  if (!nick().empty())
      server.erase_nickname(nick());
  // store new nickname
  server.hash_nickname(new_nick, get_id());
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
  handle_quit(buff, len);
  
  if (warn) {
    // close connection after write
    conn->write(buff, len,
    [this] (size_t) {
        conn->close();
    });
  } else {
    conn->close();
  }
}

void Client::handle_quit(const char* buff, int len)
{
  if (is_reg()) {
    // inform others about disconnect
    server.user_bcast_butone(get_id(), buff, len);
    // remove client from various lists
    for (size_t idx : channels()) {
      Channel& ch = server.get_channel(idx);
      ch.remove(get_id());
      
      // if the channel became empty, remove it
      if (ch.is_alive() == false)
          server.free_channel(ch);
    }
  }
}
