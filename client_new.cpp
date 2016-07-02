#include "client.hpp"
#include "ircd.hpp"
#include "tokens.hpp"
#include "umodes.hpp"
#include "cmodes.hpp"

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
      this->user_ = msg[1];
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
  uint8_t oldreg = regis;
  bool regged = is_reg();
  regis = newreg;
  // not registered before, but registered now
  if (!regged && is_reg())
  {
    printf("* Registered: %s\n", nickuserhost().c_str());
    send(RPL_WELCOME, ":Welcome to the Internet Relay Network, " + nickuserhost());
    send(RPL_YOURHOST, ":Your host is " + server.name() + ", running v1.0");
    send(RPL_CREATED, "This server was created <date>");
    send(RPL_MYINFO, server.name() + " " + server.version() + " " + umode_string + " " + cmode_string);
    
    send_motd();
    send_lusers();
  }
  else if (oldreg == 0)
  {
    auth_notice();
  }
}

void Client::send_motd()
{
  send(RPL_MOTDSTART, ":- " + server.name() + " Message of the day - ");
  const auto& motd = server.get_motd();
  for (auto& line : motd)
    send(RPL_MOTD, ":" + line);
  send(RPL_ENDOFMOTD, ":End of MOTD command");
}

void Client::send_lusers()
{
  send(RPL_LUSERCLIENT, ":There are " + std::to_string(server.get_total_clients()) +
                        " and 0 services on 1 servers");
  send(RPL_LUSEROP,       std::to_string(server.get_total_ops()) + " :operator(s) online");
  send(RPL_LUSERCHANNELS, std::to_string(server.get_total_chans()) + " :channels formed");
  send(RPL_LUSERME, ":I have " + std::to_string(server.get_total_clients()) + "clients and 1 servers");
}
