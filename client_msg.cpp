#include "client.hpp"
#include "ircd.hpp"
#include "tokens.hpp"

#include <kernel/syscalls.hpp>
void Client::handle(
    const std::string&,
    const std::vector<std::string>& msg)
{
  // in case message handler is bad
  SET_CRASH_CONTEXT("Client::handle():\n'%s'", msg[0].c_str());
  
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
    if (msg.size() > 1 && msg[1].size() > 0)
    {
      if (server.is_channel(msg[1]))
      {
        auto ch = server.channel_by_name(msg[1]);
        if (ch != NO_SUCH_CHANNEL)
        {
          auto& channel = server.get_channel(ch);
          bool joined = false;
          
          if (msg.size() < 3)
            joined = channel.join(*this);
          else
            joined = channel.join(*this, msg[2]);
          // track channel if client joined
          if (joined) channels_.push_back(ch);
        }
        else
        {
          auto ch = server.create_channel(msg[1]);
          auto key = (msg.size() < 3) ? "" : msg[2];
          server.get_channel(ch).join(*this, key);
          // track channel
          channels_.push_back(ch);
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
          bool left = false;
          
          if (msg.size() < 3)
            left = channel.part(*this);
          else
            left = channel.part(*this, msg[2]);
          // stop tracking the channel ourselves
          if (left) channels_.remove(ch);
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
  else if (cmd == TK_TOPIC)
  {
    if (msg.size() > 1)
    {
      auto ch = server.channel_by_name(msg[1]);
      if (ch != NO_SUCH_CHANNEL)
      {
        auto& channel = server.get_channel(ch);
        if (msg.size() > 2)
        {
          if (channel.is_chanop(get_id())) {
            channel.set_topic(*this, msg[2]);
          } else {
            send(ERR_CHANOPRIVSNEEDED, msg[1] + " :You're not channel operator");
          }
        }
        else
          channel.send_topic(*this);
      }
      else
        send(ERR_NOSUCHCHANNEL, msg[1] + " :No such channel");
    }
    else
      send(ERR_NEEDMOREPARAMS, cmd + " :Not enough parameters");
  }
  else if (cmd == TK_PRIVMSG)
  {
    if (msg.size() > 2)
    {
      if (server.is_channel(msg[1]))
      {
        auto ch = server.channel_by_name(msg[1]);
        if (ch != NO_SUCH_CHANNEL)
        {
          auto& channel = server.get_channel(ch);
          // check if user can broadcast to channel
          if (channel.find(get_id()) != NO_SUCH_CLIENT)
          {
            // broadcast message to channel
            channel.bcast_butone(get_id(), ":" + nickuserhost() + " " TK_PRIVMSG " " + channel.name() + " :" + msg[2]);
          }
        }
        else
          send(ERR_NOSUCHCHANNEL, msg[1] + " :No such channel");
      }
      else // assume client
      {
        auto cl = server.user_by_name(msg[1]);
        if (cl != NO_SUCH_CLIENT)
        {
          // send private message to user
          auto& client = server.get_client(cl);
          client.send_raw(":" + nickuserhost() + " " TK_PRIVMSG " " + client.nick() + " :" + msg[2]);
        }
        else {
          send(ERR_NOSUCHNICK, msg[1] + " :No such nickname");
        }
      }
    }
    else
      send(ERR_NEEDMOREPARAMS, cmd + " :Not enough parameters");
  }
  else if (cmd == TK_QUIT)
  {
    std::string reason;
    if (msg.size() > 1) reason = msg[1];
    send_quit(reason);
    return;
  }
  else
  {
    send_nonick(ERR_NOSUCHCMD, cmd + " :Unknown command");
  }
}
