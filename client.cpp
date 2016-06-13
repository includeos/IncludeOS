#include "client.hpp"

#include "ircsplit.hpp"
#include "ircd.hpp"
#include "tokens.hpp"
#include <cassert>

void Client::reset(Connection conn)
{
  this->alive_ = true;
  this->regis = 0;
  this->umodes = 0;
  this->conn = conn;
  this->nick = "";
  this->user = "";
  this->host = "";
  this->channel_list.clear();
  this->buffer = "";
}

void Client::split_message(const std::string& msg)
{
  std::string source;
  auto vec = split(msg, source);
  
  printf("[Client]: ");
  for (auto& str : vec)
  {
    printf("[%s]", str.c_str());
  }
  printf("\n");
  // ignore empty messages
  if (vec.empty()) return;
  // transform command to uppercase
  extern void transform_to_upper(std::string& str);
  transform_to_upper(vec[0]);
  // handle message
  if (this->is_reg() == false)
    handle_new(source, vec);
  else
    handle(source, vec);
}

void Client::read(const uint8_t* buf, size_t len)
{
  while (len > 0) {
    
    int search = -1;
    
    // find line ending
    for (size_t i = 0; i < len; i++)
    if (buf[i] == 13 || buf[i] == 10) {
      search = i; break;
    }
    
    // not found:
    if (search == -1)
    {
      // append entire buffer
      buffer.append((char*) buf, len);
      break;
    }
    else {
      // found CR LF:
      if (search != 0) {
        // append to clients buffer
        buffer.append((char*) buf, search);
  
        // move forward in socket buffer
        buf += search;
        // decrease len
        len -= search;
      }
      else {
        buf++; len--;
      }
  
      // parse message
      if (buffer.size())
      {
        split_message(buffer);
        buffer.clear();
      }
    }
  }
}

void Client::send(uint16_t numeric, std::string text)
{
  std::string num;
  num.reserve(128);
  num = std::to_string(numeric);
  num = std::string(3 - num.size(), '0') + num;
  
  num = ":" + server.name() + " " + num + " " + this->nick + " " + text + "\r\n";
  //printf("-> %s", num.c_str());
  conn->write(num.c_str(), num.size());
}
void Client::send(std::string text)
{
  std::string data = ":" + server.name() + " " + text + "\r\n";
  //printf("-> %s", data.c_str());
  conn->write(data.c_str(), data.size());
}

// validate name, returns false if invalid characters
static bool validate_name(const std::string& new_name)
{
  // a-z A-Z 0-9 _ - \ [ ] { } ^ ` |
  static const std::string LUT = 
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890_-\\[]{}^`|ÆØÅæøå";
  static const std::string FLUT = 
    "01234567890";
  // empty nickname is invalid
  assert(!new_name.empty());
  // forbidden first characters
  for (char c : FLUT)
      if (new_name[0] == c) return false;
  // forbidden first characters
  for (size_t i = 0; i < new_name.size(); i++)
  {
    bool found = false;
    for (char c : LUT)
    if (new_name[i] == c) {
      found = true;
      break;
    }
    if (found == false) return false;
  }
  return true;
}
bool Client::change_nick(const std::string& new_nick)
{
  if (new_nick.size() < server.nick_minlen()) {
    send(ERR_ERRONEUSNICKNAME, new_nick + " :Nickname too short");
    return false;
  }
  if (new_nick.size() > server.nick_maxlen()) {
    send(ERR_ERRONEUSNICKNAME, new_nick + " :Nickname too long");
    return false;
  }
  if (!validate_name(new_nick)) {
    send(ERR_ERRONEUSNICKNAME, new_nick + " :Erroneous nickname");
    return false;
  }
  auto idx = server.user_by_name(new_nick);
  if (idx != NO_SUCH_CLIENT) {
    send(ERR_NICKNAMEINUSE, new_nick + " :Nickname is already in use");
    return false;
  }
  // nickname is valid and free, take it
  this->nick = new_nick;
  return true;
}
