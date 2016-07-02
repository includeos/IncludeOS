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
    {
      if (server.is_channel(msg[1]))
      {
        auto ch = server.channel_by_name(msg[1]);
        if (ch != NO_SUCH_CHANNEL)
        {
          auto& channel = server.get_channel(ch);
          channel.send_mode(*this);
        }
        else
        {
          send(ERR_NOSUCHCHANNEL, msg[1] + " :No such channel");
        }
      }
      else {
        // non-ircops can only usermode themselves
        if (msg[1] == this->nick())
          send(RPL_UMODEIS, "+i");
        else
          send(ERR_USERSDONTMATCH, ":Cannot change mode for other users");
      }
    }
    else
      send(ERR_NEEDMOREPARAMS, cmd + " :Not enough parameters");
  }
  else if (cmd == TK_JOIN)
  {
    if (msg.size() > 1)
    {
      if (server.is_channel(msg[1]))
      {
        auto ch = server.channel_by_name(msg[1]);
        if (ch != NO_SUCH_CHANNEL)
        {
          auto& channel = server.get_channel(ch);
          if (msg.size() < 3)
            channel.join(*this);
          else
            channel.join(*this, msg[2]);
        }
        else
        {
          auto ch = server.create_channel(msg[1]);
          auto key = (msg.size() < 3) ? "" : msg[2];
          server.get_channel(ch).join(*this, key);
        }
      }
      else {
        send(ERR_NOSUCHCHANNEL, msg[1] + " :No such channel");
      }
    }
    else
      send(ERR_NEEDMOREPARAMS, cmd + " :Not enough parameters");
  }
  else if (cmd == TK_PART)
  {
    if (msg.size() > 1)
    {
      if (server.is_channel(msg[1]))
      {
        auto ch = server.channel_by_name(msg[1]);
        if (ch != NO_SUCH_CHANNEL)
        {
          auto& channel = server.get_channel(ch);
          if (msg.size() < 3)
            channel.part(*this);
          else
            channel.part(*this, msg[2]);
        }
        else
          send(ERR_NOSUCHCHANNEL, msg[1] + " :No such channel");
      }
      else {
        send(ERR_NOSUCHCHANNEL, msg[1] + " :No such channel");
      }
    }
    else
      send(ERR_NEEDMOREPARAMS, cmd + " :Not enough parameters");
  }
  else if (cmd == TK_QUIT)
  {
    
  }
  else
  {
    send_nonick(ERR_NOSUCHCMD, cmd + " :Unknown command");
  }
}
