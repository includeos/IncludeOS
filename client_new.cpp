#include "client.hpp"
#include "ircd.hpp"
#include "tokens.hpp"
#include "modes.hpp"

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
        welcome(2);
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
      this->user_ = msg[1];
      welcome(4);
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

void Client::auth_notice()
{
  send("NOTICE AUTH :*** Processing your connection");
  send("NOTICE AUTH :*** Looking up your hostname...");
  //hostname_lookup()
  send("NOTICE AUTH :*** Checking Ident");
  //ident_check()
  this->host_ = conn->remote().address().str();
}
void Client::welcome(uint8_t newreg)
{
  bool regged = is_reg();
  regis |= newreg;
  // not registered before, but registered now
  if (!regged && is_reg())
  {
    send(RPL_WELCOME, ":Welcome to the Internet Relay Network, " + nickuserhost());
    send(RPL_YOURHOST, ":Your host is " + server.name() + ", running v1.0");
    send(RPL_CREATED, ":This server was created <date>");
    send(RPL_MYINFO, server.name() + " " + server.version() + " " + usermodes.get());
    send(RPL_CAPABS, "CHANTYPES=&# EXCEPTS PREFIX=(ov)@+ CHANMODES=eIb,k,l,imnpstu :are supported by this server");
    send(RPL_CAPABS, "NETWORK=" + server.network() + " NICKLEN=" + std::to_string(server.nick_maxlen()) + " CHANNELLEN=" + std::to_string(server.chan_maxlen()) + " :are supported by this server");
    send_motd();
    send_lusers();
    send_modes();
  }
}
