#include "client.hpp"
#include "ircd.hpp"
#include "tokens.hpp"
#include "modes.hpp"
#include <profile>

inline void Client::need_parms(const std::string& cmd)
{
  send(ERR_NEEDMOREPARAMS, cmd + " :Not enough parameters");
}

void Client::handle_new(const std::vector<std::string>& msg)
{
#ifndef USERSPACE_LINUX
  volatile ScopedProfiler profile;
#endif
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
      need_parms(cmd);
  }
  else if (cmd == TK_NICK)
  {
    if (msg.size() > 1)
    {
      // try to acquire nickname
      if (change_nick(msg[1])) {
        welcome(2);
      }
    }
    else
      need_parms(cmd);
  }
  else if (cmd == TK_USER)
  {
    if (msg.size() > 1)
    {
      this->user_ = msg[1];
      welcome(4);
    }
    else
      need_parms(cmd);
  }
  else
  {
    send(ERR_NOSUCHCMD, cmd + " :Unknown command");
  }
}

void Client::auth_notice()
{
  static const char auth_proc[] =
      "NOTICE AUTH :*** Processing your connection\r\n";
  static const char auth_host[] =
      "NOTICE AUTH :*** Looking up your hostname...\r\n";
  static const char auth_idnt[] =
      "NOTICE AUTH :*** Checking Ident\r\n";
  send_raw(auth_proc, sizeof auth_proc - 1);
  send_raw(auth_host, sizeof auth_host - 1);
  send_raw(auth_idnt, sizeof auth_idnt - 1);
  //hostname_lookup()
  this->host_ = conn->remote().address().to_string();
  //ident_check()
}
void Client::welcome(uint8_t newreg)
{
  bool regged = is_reg();
  regis |= newreg;
  // not registered before, but registered now
  if (!regged && is_reg())
  {
    // statistics
    server.new_registered_client();
    // welcoming messages
    send(RPL_WELCOME, ":Welcome to the Internet Relay Network, " + nickuserhost());
    send(RPL_YOURHOST, ":Your host is " + server.name() + ", running " IRC_SERVER_VERSION);
    send(RPL_CREATED, ":This server was created " + server.created());
    send(RPL_MYINFO, server.name() + " " + server.version() + " " + usermodes.get());
    send(RPL_CAPABS, "CHANTYPES=&# EXCEPTS PREFIX=(ov)@+ CHANMODES=eIb,k,l,imnpstu :are supported by this server");
    send(RPL_CAPABS, "NETWORK=" + server.network() + " NICKLEN=" + std::to_string(server.nick_maxlen()) + " CHANNELLEN=" + std::to_string(server.chan_maxlen()) + " :are supported by this server");
    send_motd();
    send_lusers();
    send_modes();
  }
}
