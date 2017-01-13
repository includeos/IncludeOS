#include "server.hpp"

#include "ircd.hpp"
#include "ircsplit.hpp"

static const uint8_t REGIS_SERV = 2;
static const uint8_t REGIS_PASS = 4;

Server::Server(sindex_t idx, IrcServer& srv)
  : self(idx), regis(0), token_(0), server(srv) {}

void Server::connect(Connection conn)
{
  this->regis      = 0;
  this->near_link_ = server.server_id();
  this->conn       = conn;
  this->remote_links.clear();
  setup_dg();
}
void Server::connect(Connection conn, std::string name, std::string pass)
{
  this->regis      = 0;
  this->near_link_ = server.server_id();
  this->sname = name;
  this->spass = pass;
  this->conn  = conn;
  this->remote_links.clear();
  setup_dg();
  
  conn->on_connect(
  [ircd = &server, id = get_id()] (auto conn) {
    /// introduce ourselves
    printf("SERVER CONNECTED TO %s\n", conn->remote().to_string().c_str());
    auto& srv = ircd->servers.get(id);
    srv.send("SERVER " + ircd->name() + "\r\n");
    srv.send("PASS :" + srv.get_pass() + "\r\n");
  });
}
void Server::setup_dg()
{
  /// wait for acceptance ...
  conn->on_read(512,
  [ircd = &server, id = get_id()] (auto buf, size_t len) {
    auto& srv = ircd->servers.get(id);
    srv.readq.read(buf.get(), len, {srv, &Server::split_message});
  });
}
void Server::split_message(const std::string& text)
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
      printf("Received SERVER: %s\n", msg[1].c_str());
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

void Server::send(const std::string& str)
{
  conn->write(str.c_str(), str.size());
}
void Server::send(const char* str, size_t len)
{
  conn->write(str, len);
}

void Server::squit(const std::string& reason)
{
  // local servers have connections
  if (conn)
  {
    conn->write("SQUIT :" + reason);
    conn->close();
    /// create netsplit reason message, propagate to servers and users
    std::string netsplit = "Netsplit: " + server.name() + "<->" + this->name();
    /// remove all servers at or behind this server
    for (auto srv : remote_links)
    {
      server.servers.get(srv).squit(netsplit);
    }
    /// remove this server
    this->regis = 0;
    server.servers.free(*this);
    /// remove clients on this server
    server.kill_remote_clients_on(get_id(), netsplit);
  }
  else
  {
    /// remove remote clients on this server
    server.kill_remote_clients_on(get_id(), reason);
  }
}
