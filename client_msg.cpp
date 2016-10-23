#include "client.hpp"
#include "ircd.hpp"
#include "tokens.hpp"

inline void Client::not_ircop(const std::string& cmd)
{
  send(ERR_NOPRIVILEGES, cmd + " :Permission Denied- You're not an IRC operator");
}
inline void Client::need_parms(const std::string& cmd)
{
  send(ERR_NEEDMOREPARAMS, cmd + " :Not enough parameters");
}

#include <kernel/syscalls.hpp>
void Client::handle(
    const std::string&,
    const std::vector<std::string>& msg)
{
  // in case message handler is bad
  #define BUFFER_SIZE   1024
  char buffer[BUFFER_SIZE];
  
  const std::string& cmd = msg[0];
  
  if (cmd == TK_PING)
  {
    if (msg.size() > 1) {
      int len = snprintf(buffer, sizeof(buffer),
          "PONG :%s\r\n", msg[1].c_str());
      send_raw(buffer, len);
    }
    else
      need_parms(cmd);
  }
  else if (cmd == TK_PONG)
  {
    // do nothing
  }
  else if (cmd == TK_PASS)
  {
    need_parms(cmd);
  }
  else if (cmd == TK_NICK)
  {
    if (msg.size() > 1)
    {
      const std::string nuh = nickuserhost();
      // change nickname
      if (change_nick(msg[1])) {
        // success, broadcast to all who can see client
        int len = snprintf(buffer, sizeof(buffer),
                  ":%s NICK %s\r\n", nuh.c_str(), nick().c_str());
        server.user_bcast(get_id(), buffer, len);
      }
    }
    else
      need_parms(cmd);
  }
  else if (cmd == TK_USER)
  {
    send(ERR_NOSUCHCMD, cmd + " :Unknown command");
  }
  else if (cmd == TK_MOTD)
  {
    send_motd();
  }
  else if (cmd == TK_LUSERS)
  {
    send_lusers();
  }
  else if (cmd == TK_STATS)
  {
    if (msg.size() > 1)
    {
      send_stats(msg[1]);
    }
    else
      need_parms(cmd);
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
      need_parms(cmd);
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
          // there is also a maximum number of channels a user can join
          if (channels().size() < server.client_maxchans() || is_operator())
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
            // joined too many channels
            send(ERR_TOOMANYCHANNELS, msg[1] + " :You have joined too many channels");
          }
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
      need_parms(cmd);
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
          if (left) {
            channels_.remove(ch);
            // if the channel became empty, remove it
            if (channel.is_alive() == false)
                server.free_channel(channel);
          }
        }
        else
          send(ERR_NOSUCHCHANNEL, msg[1] + " :No such channel");
      }
      else {
        send(ERR_NOSUCHCHANNEL, msg[1] + " :No such channel");
      }
    }
    else
      need_parms(cmd);
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
      need_parms(cmd);
  }
  else if (cmd == TK_NAMES)
  {
    if (msg.size() > 1)
    {
      auto ch = server.channel_by_name(msg[1]);
      if (ch != NO_SUCH_CHANNEL)
      {
        auto& channel = server.get_channel(ch);
        if (channel.find(self) != NO_SUCH_CLIENT) {
            channel.send_names(*this);
        }
        else
            send(ERR_NOTONCHANNEL, msg[1] + " :You're not on that channel");
      }
      else
        send(ERR_NOSUCHCHANNEL, msg[1] + " :No such channel");
    }
    else
      need_parms(cmd);
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
          if (channel.find(self) != NO_SUCH_CLIENT)
          {
            // broadcast message to channel
            int len = snprintf(buffer, sizeof(buffer),
                      ":%s PRIVMSG %s :%s\r\n",
                      nickuserhost().c_str(), channel.name().c_str(), msg[2].c_str());
            channel.bcast_butone(get_id(), buffer, len);
          }
          else
              send(ERR_NOTONCHANNEL, msg[1] + " :You're not on that channel");
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
          client.send_from(nickuserhost(), TK_PRIVMSG " " + client.nick() + " :" + msg[2]);
        }
        else
          send(ERR_NOSUCHNICK, msg[1] + " :No such nickname");
      }
    }
    else
      need_parms(cmd);
  }
  else if (cmd == TK_QUIT)
  {
    std::string reason("Quit");
    if (msg.size() > 1) reason = "Quit: " + msg[1];
    kill(true, reason);
    return;
  }
  else if (cmd == TK_SVSHOST)
  {
    if (is_operator())
    {
      if (msg.size() > 2)
      {
        auto cl = server.user_by_name(msg[1]);
        if (cl != NO_SUCH_CLIENT) {
          auto& client = server.get_client(cl);
          // TODO: validate host (must contain at least one .)
          if (msg[2].size() > 2)
              client.set_vhost(msg[2]);
        }
        else
          send(ERR_NOSUCHNICK, msg[1] + " :No such nickname");
      }
      else
        need_parms(cmd);
    }
    else
      not_ircop(cmd);
  }
  else
  {
    send(ERR_NOSUCHCMD, cmd + " :Unknown command");
  }
}
