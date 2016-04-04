#include "ircd.hpp"
#include "ircsplit.hpp"

static const std::string SERVER_NAME = "irc.includeos.org";

std::vector<Client> clients;

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
  if (vec.size() == 0) return;
  // handle message
  handle(source, vec);
}

void Client::read(const char* buf, size_t len)
{
  while (len > 0)
    {
      int search = -1;
    
      for (size_t i = 0; i < len; i++)
        if (buf[i] == 13 || buf[i] == 10)
          {
            search = i; break;
          }
      // not found:
      if (search == -1)
        {
          // append entire buffer
          buffer.append(buf, len);
          break;
        }
      else
        {
          // found CR LF:
          if (search != 0)
            {
              // append to clients buffer
              buffer.append(buf, search);
        
              // move forward in socket buffer
              buf += search;
              // decrease len
              len -= search;
            }
          else
            {
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
  
  num = ":" + SERVER_NAME + " " + num + " " + this->nick + " " + text + "\r\n";
  //printf("-> %s", num.c_str());
  conn->write(num.c_str(), num.size());
}
void Client::send(std::string text)
{
  std::string data = ":" + SERVER_NAME + " " + text + "\r\n";
  //printf("-> %s", data.c_str());
  conn->write(data.c_str(), data.size());
}

#define ERR_NOSUCHNICK     401
#define ERR_NOSUCHCMD      421
#define ERR_NEEDMOREPARAMS 461


void Client::handle(const std::string&,
                    const std::vector<std::string>& msg)
{
#define TK_CAP    "CAP"
#define TK_PASS   "PASS"
#define TK_NICK   "NICK"
#define TK_USER   "USER"
  
  const std::string& cmd = msg[0];
  
  if (this->is_reg() == false)
    {
      if (cmd == TK_CAP)
        {
          // ignored completely
        }
      else if (cmd == TK_PASS)
        {
          if (msg.size() > 1)
            {
              this->passw = msg[1];
            }
          else
            {
              send(ERR_NEEDMOREPARAMS, cmd + " :Not enough parameters");
            }
        }
      else if (cmd == TK_NICK)
        {
          if (msg.size() > 1)
            {
              this->nick = msg[1];
              welcome(regis | 1);
            }
          else
            {
              send(ERR_NEEDMOREPARAMS, cmd + " :Not enough parameters");
            }
        }
      else if (cmd == TK_USER)
        {
          if (msg.size() > 1)
            {
              this->user = msg[1];
              welcome(regis | 2);
            }
          else
            {
              send(ERR_NEEDMOREPARAMS, cmd + " :Not enough parameters");
            }
        }
      else
        {
          send(ERR_NOSUCHCMD, cmd + " :Unknown command");
        }
    }
}

#define RPL_WELCOME   1
#define RPL_YOURHOST  2
#define RPL_CREATED   3
#define RPL_MYINFO    4
#define RPL_BOUNCE    5

void Client::welcome(uint8_t newreg)
{
  uint8_t oldreg = regis;
  bool regged = is_reg();
  regis = newreg;
  // not registered before, but registered now
  if (!regged && is_reg())
    {
      printf("* Registered: %s\n", nickuserhost().c_str());
      send(RPL_WELCOME, ":Welcome to the Internet Relay Network, " + nickuserhost());
      send(RPL_YOURHOST, ":Your host is " + SERVER_NAME + ", running v1.0");
    }
  else if (oldreg == 0)
    {
      auth_notice();
    }
}
void Client::auth_notice()
{
  send("NOTICE AUTH :*** Processing your connection..");
  send("NOTICE AUTH :*** Looking up your hostname...");
  //hostname_lookup()
  send("NOTICE AUTH :*** Checking Ident");
  //ident_check()
}
