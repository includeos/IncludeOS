#include "client.hpp"
#include "ircd.hpp"
#include "tokens.hpp"

void Client::handle(
    const std::string&,
    const std::vector<std::string>& msg)
{
  const std::string& cmd = msg[0];
  
  if (cmd == TK_PING)
  {
    if (msg.size() > 1)
      send("PONG :" + msg[1]);
    else
      send(ERR_NEEDMOREPARAMS, cmd + " :Not enough parameters");
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
    send_nonick(ERR_NOSUCHCMD, cmd + " :Unknown command");
  }
  else if (cmd == TK_MOTD)
  {
    send_motd();
  }
  else if (cmd == TK_LUSERS)
  {
    send_lusers();
  }
  else if (cmd == TK_USERHOST)
  {
    send(RPL_USERHOST, " = " + userhost());
  }
  else if (cmd == TK_MODE)
  {
    if (msg.size() > 1)
      // non-ircops can only usermode themselves
      if (msg[1] == this->nick())
        send(RPL_UMODEIS, "+i");
      else
        send(ERR_USERSDONTMATCH, ":Cannot change mode for other users");
    else
      send(ERR_NEEDMOREPARAMS, cmd + " :Not enough parameters");
  }
  else if (cmd == TK_JOIN)
  {
    
  }
  else if (cmd == TK_QUIT)
  {
    
  }
  else
  {
    send_nonick(ERR_NOSUCHCMD, cmd + " :Unknown command");
  }
}
