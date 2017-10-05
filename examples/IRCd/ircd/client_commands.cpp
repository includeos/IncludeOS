#include "client.hpp"
#include "ircd.hpp"
#include "tokens.hpp"
#include <unordered_map>
#define BUFFER_SIZE   1024

typedef delegate<void(Client&, const std::vector<std::string>&)> command_func_t;
static std::map<std::string, command_func_t> funcs;

inline void Client::not_ircop(const std::string& cmd)
{
  send(ERR_NOPRIVILEGES, cmd + " :Permission Denied- You're not an IRC operator");
}
inline void Client::need_parms(const std::string& cmd)
{
  send(ERR_NEEDMOREPARAMS, cmd + " :Not enough parameters");
}

static void handle_ping(Client& client, const std::vector<std::string>& msg)
{
  if (msg.size() > 1) {
    char buffer[64];
    int len = snprintf(buffer, sizeof(buffer),
        "PONG :%s\r\n", msg[1].c_str());
    if (len > 0) client.send_raw(buffer, len);
  }
  else
    client.need_parms(msg[0]);
}
static void handle_pong(Client&, const std::vector<std::string>&)
{
  // do nothing
}
static void handle_pass(Client&, const std::vector<std::string>&)
{
  // do nothing
}
static void handle_nick(Client& client, const std::vector<std::string>& msg)
{
  if (msg.size() > 1)
  {
    const std::string nuh = client.nickuserhost();
    // attempt to change nickname
    if (client.change_nick(msg[1]))
    {
      // success, broadcast to all who can see client
      char buffer[BUFFER_SIZE];
      int len = snprintf(buffer, sizeof(buffer),
                ":%s NICK %s\r\n", nuh.c_str(), client.nick().c_str());
      auto& server = client.get_server();
      server.user_bcast(client.get_id(), buffer, len);
    }
  }
  else
    client.need_parms(msg[0]);
}
static void handle_user(Client& client, const std::vector<std::string>& msg)
{
  client.send(ERR_NOSUCHCMD, msg[0] + " :Unknown command");
}
static void handle_motd(Client& client, const std::vector<std::string>&)
{
  client.send_motd();
}
static void handle_lusers(Client& client, const std::vector<std::string>&)
{
  client.send_lusers();
}
static void handle_stats(Client& client, const std::vector<std::string>& msg)
{
  if (msg.size() > 1)
  {
    client.send_stats(msg[1]);
  }
  else
    client.need_parms(msg[0]);
}

static void handle_userhost(Client& client, const std::vector<std::string>& msg)
{
  client.send(RPL_USERHOST, " = " + client.userhost());
}
static void handle_whois(Client& client, const std::vector<std::string>& msg)
{
  // implement me
  client.need_parms(msg[0]);
}
static void handle_who(Client& client, const std::vector<std::string>& msg)
{
  if (msg.size() > 1)
  {
    auto& server = client.get_server();
    auto cl = server.clients.find(msg[1]);

    if (cl != NO_SUCH_CLIENT)
    {
      auto& other = server.clients.get(cl);

      char buffer[BUFFER_SIZE];
      int len = snprintf(buffer, sizeof(buffer),
                ":%s 352 %s * %s %s %s %s H :0 %s\r\n",
                server.name().c_str(),
                client.nick().c_str(),
                other.user().c_str(),
                other.host().c_str(),
                server.name().c_str(), /// FIXME users server
                other.nick().c_str(),
                "Realname"); //other.realname()
      client.send_raw(buffer, len);
      len = snprintf(buffer, sizeof(buffer),
                ":%s 315 %s %s :End of /WHO list\r\n",
                server.name().c_str(),
                client.nick().c_str(),
                msg[1].c_str());
      client.send_raw(buffer, len);
    }
    else
      client.send(ERR_NOSUCHNICK, msg[1] + " :No such nickname");
  }
  else
    client.need_parms(msg[0]);
}


static void handle_mode(Client& client, const std::vector<std::string>& msg)
{
  if (msg.size() > 1)
  {
    auto& server = client.get_server();
    if (server.is_channel(msg[1]))
    {
      auto ch = server.channels.find(msg[1]);
      if (ch != NO_SUCH_CHANNEL)
      {
        auto& channel = server.channels.get(ch);
        channel.send_mode(client);
      }
      else
      {
        client.send(ERR_NOSUCHCHANNEL, msg[1] + " :No such channel");
      }
    }
    else {
      // non-ircops can only usermode themselves
      if (msg[1] == client.nick())
        client.send(RPL_UMODEIS, "+i");
      else
        client.send(ERR_USERSDONTMATCH, ":Cannot change mode for other users");
    }
  }
  else
    client.need_parms(msg[0]);
}
static void handle_join(Client& client, const std::vector<std::string>& msg)
{
  if (msg.size() > 1 && msg[1].size() > 0)
  {
    auto& server = client.get_server();
    if (server.is_channel(msg[1]))
    {
      auto ch = server.channels.find(msg[1]);
      if (ch != NO_SUCH_CHANNEL)
      {
        // there is also a maximum number of channels a user can join
        if (client.channels().size() < server.client_maxchans() || client.is_operator())
        {
          auto& channel = server.channels.get(ch);
          bool joined = false;

          if (msg.size() < 3)
            joined = channel.join(client);
          else
            joined = channel.join(client, msg[2]);
          // track channel if client joined
          if (joined) client.channels().push_back(ch);
        }
        else
        {
          // joined too many channels
          client.send(ERR_TOOMANYCHANNELS, msg[1] + " :You have joined too many channels");
        }
      }
      else
      {
        auto ch = server.create_channel(msg[1]);
        auto key = (msg.size() < 3) ? "" : msg[2];
        server.channels.get(ch).join(client, key);
        // track channel
        client.channels().push_back(ch);
      }
    }
    else {
      client.send(ERR_NOSUCHCHANNEL, msg[1] + " :No such channel");
    }
  }
  else
    client.need_parms(msg[0]);
}
static void handle_part(Client& client, const std::vector<std::string>& msg)
{
  if (msg.size() > 1)
  {
    auto& server = client.get_server();
    if (server.is_channel(msg[1]))
    {
      auto ch = server.channels.find(msg[1]);
      if (ch != NO_SUCH_CHANNEL)
      {
        auto& channel = server.channels.get(ch);
        bool left = false;

        if (msg.size() < 3)
          left = channel.part(client);
        else
          left = channel.part(client, msg[2]);
        // stop tracking the channel ourselves
        if (left) {
          client.channels().remove(ch);
          // if the channel became empty, remove it
          if (channel.is_alive() == false)
              server.free_channel(channel);
        }
      }
      else
        client.send(ERR_NOSUCHCHANNEL, msg[1] + " :No such channel");
    }
    else {
      client.send(ERR_NOSUCHCHANNEL, msg[1] + " :No such channel");
    }
  }
  else
    client.need_parms(msg[0]);
}
static void handle_topic(Client& client, const std::vector<std::string>& msg)
{
  if (msg.size() > 1)
  {
    auto& server = client.get_server();
    auto ch = server.channels.find(msg[1]);
    if (ch != NO_SUCH_CHANNEL)
    {
      auto& channel = server.channels.get(ch);
      if (msg.size() > 2)
      {
        if (channel.is_chanop(client.get_id())) {
          channel.set_topic(client, msg[2]);
        } else {
          client.send(ERR_CHANOPRIVSNEEDED, msg[1] + " :You're not channel operator");
        }
      }
      else
        channel.send_topic(client);
    }
    else
      client.send(ERR_NOSUCHCHANNEL, msg[1] + " :No such channel");
  }
  else
    client.need_parms(msg[0]);
}
static void handle_names(Client& client, const std::vector<std::string>& msg)
{
  if (msg.size() > 1)
  {
    auto& server = client.get_server();
    auto ch = server.channels.find(msg[1]);
    if (ch != NO_SUCH_CHANNEL)
    {
      auto& channel = server.channels.get(ch);
      if (channel.find(client.get_id()) != NO_SUCH_CLIENT) {
          channel.send_names(client);
      }
      else
          client.send(ERR_NOTONCHANNEL, msg[1] + " :You're not on that channel");
    }
    else
      client.send(ERR_NOSUCHCHANNEL, msg[1] + " :No such channel");
  }
  else
    client.need_parms(msg[0]);
}
static void handle_privmsg(Client& client, const std::vector<std::string>& msg)
{
  if (msg.size() > 2)
  {
    auto& server = client.get_server();
    if (server.is_channel(msg[1]))
    {
      auto ch = server.channels.find(msg[1]);
      if (ch != NO_SUCH_CHANNEL)
      {
        auto& channel = server.channels.get(ch);
        // check if user can broadcast to channel
        if (channel.find(client.get_id()) != NO_SUCH_CLIENT)
        {
          // broadcast message to channel
          char buffer[BUFFER_SIZE];
          int len = snprintf(buffer, sizeof(buffer),
                    ":%s PRIVMSG %s :%s\r\n",
                    client.nickuserhost().c_str(), channel.name().c_str(), msg[2].c_str());
          channel.bcast_butone(client.get_id(), buffer, len);
        }
        else
            client.send(ERR_NOTONCHANNEL, msg[1] + " :You're not on that channel");
      }
      else
        client.send(ERR_NOSUCHCHANNEL, msg[1] + " :No such channel");
    }
    else // assume client
    {
      auto cl = server.clients.find(msg[1]);
      if (cl != NO_SUCH_CLIENT)
      {
        // send private message to user
        auto& other = server.clients.get(cl);
        other.send_from(client.nickuserhost(), TK_PRIVMSG " " + other.nick() + " :" + msg[2]);
      }
      else
        client.send(ERR_NOSUCHNICK, msg[1] + " :No such nickname");
    }
  }
  else
    client.need_parms(msg[0]);
}
static void handle_quit(Client& client, const std::vector<std::string>& msg)
{
  std::string reason("Quit");
  if (msg.size() > 1) reason = "Quit: " + msg[1];
  client.kill(true, reason);
}

/// services related ///
static void handle_kill(Client& client, const std::vector<std::string>& msg)
{
  if (client.is_operator())
  {
    if (msg.size() > 1)
    {
      auto& server = client.get_server();
      auto cl = server.clients.find(msg[1]);
      if (cl != NO_SUCH_CLIENT)
      {
        auto& other = server.clients.get(cl);

        std::string reason = "Killed by " + client.nick();
        if (msg.size() > 2) reason += ": " + msg[2];
        other.kill(true, reason);
      }
      else
        client.send(ERR_NOSUCHNICK, msg[1] + " :No such nickname");

    }
    else
      client.need_parms(msg[0]);
  }
  else
    client.not_ircop(msg[0]);
}
static void handle_svsnick(Client& client, const std::vector<std::string>& msg)
{
}
static void handle_svshost(Client& client, const std::vector<std::string>& msg)
{
  if (client.is_operator())
  {
    if (msg.size() > 2)
    {
      auto& server = client.get_server();
      auto cl = server.clients.find(msg[1]);
      if (cl != NO_SUCH_CLIENT) {
        auto& client = server.clients.get(cl);
        // TODO: validate host (must contain at least one .)
        if (msg[2].size() > 2)
            client.set_vhost(msg[2]);
      }
      else
        client.send(ERR_NOSUCHNICK, msg[1] + " :No such nickname");
    }
    else
      client.need_parms(msg[0]);
  }
  else
    client.not_ircop(msg[0]);
}
static void handle_svsjoin(Client& client, const std::vector<std::string>& msg)
{
}

static void handle_version(Client& client, const std::vector<std::string>& msg)
{
  client.send(RPL_VERSION, "IncludeOS IRCd " IRC_SERVER_VERSION);
}
static void handle_admin(Client& client, const std::vector<std::string>& msg)
{
  client.send(RPL_ADMINME, ":Administrative info about " + client.get_server().name());
  client.send(RPL_ADMINLOC1, ":");
  client.send(RPL_ADMINLOC2, ":");
  client.send(RPL_ADMINEMAIL, "contact@staff.irc");
}

void Client::handle_cmd(const std::vector<std::string>& msg)
{
  auto it = funcs.find(msg[0]);
  if (it != funcs.end()) {
    it->second(*this, msg);
  }
  else {
    this->send(ERR_NOSUCHCMD, msg[0] + " :Unknown command");
  }
}

void Client::init()
{
  funcs["PING"] = handle_ping;
  funcs["PONG"] = handle_pong;
  funcs["PASS"] = handle_pass;

  funcs["NICK"] = handle_nick;
  funcs["USER"] = handle_user;
  funcs["MOTD"] = handle_motd;
  funcs["LUSERS"] = handle_lusers;
  funcs["STATS"]  = handle_stats;
  funcs["MODE"] = handle_mode;

  funcs["USERHOST"] = handle_userhost;
  funcs["WHOIS"]    = handle_whois;
  funcs["WHO"]      = handle_who;

  funcs["JOIN"]  = handle_join;
  funcs["PART"]  = handle_part;
  funcs["TOPIC"] = handle_topic;
  funcs["NAMES"] = handle_names;
  funcs["PRIVMSG"] = handle_privmsg;
  funcs["QUIT"]  = handle_quit;

  funcs["KILL"]  = handle_kill;
  funcs["SVSNICK"]  = handle_svsnick;
  funcs["SVSHOST"]  = handle_svshost;
  funcs["SVSJOIN"]  = handle_svsjoin;

  funcs["VERSION"]  = handle_version;
  funcs["ADMIN"]    = handle_admin;
}
