#include "client.hpp"
#include "ircd.hpp"
#include "tokens.hpp"

void Client::handle_new(
    const std::string&,
    const std::vector<std::string>& msg)
{
  const std::string& cmd = msg[0];
  
  if (cmd == TK_CAP)
  {
    // ignored completely
  }
  else if (cmd == TK_PASS)
  {
    if (msg.size() > 1)
    {
      // ignore passwords for now
      //this->passw = msg[1];
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
      // try to acquire nickname
      if (change_nick(msg[1])) {
        welcome(regis | 1);
      }
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
    send(RPL_YOURHOST, ":Your host is " + server.name() + ", running v1.0");
  }
  else if (oldreg == 0)
  {
    auth_notice();
  }
}
void Client::auth_notice()
{
  send("NOTICE AUTH :*** Processing your connection");
  send("NOTICE AUTH :*** Looking up your hostname...");
  //hostname_lookup()
  send("NOTICE AUTH :*** Checking Ident");
  //ident_check()
  this->host = conn->remote().address().str();
}

void Client::handle(
    const std::string&,
    const std::vector<std::string>& msg)
{
  const std::string& cmd = msg[0];
  
  if (cmd == TK_PING)
  {
    if (msg.size() > 1)
    {
      send("PONG :" + msg[1]);
    }
    else
    {
      send(ERR_NEEDMOREPARAMS, cmd + " :Not enough parameters");
    }
  }
  else if (cmd == TK_PASS)
  {
    send(ERR_NEEDMOREPARAMS, cmd + " :Not enough parameters");
  }
  else if (cmd == TK_NICK)
  {
    if (msg.size() > 1)
    {
      // change nickname
      change_nick(msg[1]);
    }
    else
    {
      send(ERR_NEEDMOREPARAMS, cmd + " :Not enough parameters");
    }
  }
  else if (cmd == TK_USER)
  {
    send(ERR_NEEDMOREPARAMS, cmd + " :Not enough parameters");
  }
  else
  {
    send(ERR_NOSUCHCMD, cmd + " :Unknown command");
  }
}
