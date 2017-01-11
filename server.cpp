#include "server.hpp"

#include "ircd.hpp"
#include "ircsplit.hpp"

static const uint8_t REGIS_SERV = 2;
static const uint8_t REGIS_PASS = 4;

Server::Server(sindex_t idx, IrcServer& srv)
  : self(idx), regis(0), server(srv)
{
  
}

void Server::recv(const std::string& text)
{
  auto msg = ircsplit(text);
  // just skip empty messages
  if (msg.empty()) return;

  if (is_regged()) {
    handle_commands(msg);
  }
  else {
    handle_unknown(msg);
  }
}

void Server::handle_unknown(const std::vector<std::string>& msg)
{
  if (msg[0] == "SERVER")
  {
    /// register servername etc
    if (msg.size() > 4) {
      /// SERVER [SERVER_NAME] [HOP_COUNT] [BOOT_TS] [LINK_TS] [PROTOCOL] [NUMERIC/MAXCONN] [+FLAGS] :[DESCRIPTION]
      this->sname  = msg[1];
      this->regis |= REGIS_SERV;
      try_auth();
    }
    else {
      squit("Invalid command");
    }
  }
  else if (msg[0] == "PASS")
  {
    /// register password
    if (msg.size() > 1) {
      this->spass  = msg[1];
      this->regis |= REGIS_PASS;
      try_auth();
    }
    else {
      squit("Invalid command");
    }
  }
  else {
    squit("Invalid command");
  }
}

void Server::try_auth()
{
  if (regis == 7) {
    /// validate server
    if (server.accept_remote_server(this->sname, this->spass) == false)
    {
      squit("Unknown server");
      return;
    }
    /// enter netburst mode
    
  }
}

void Server::squit(const std::string& reason)
{
  if (conn) {
    conn->write("QUIt :" + reason);
    conn->close();
  }
  /// remove server from ircd
  
}
