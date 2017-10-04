#include "server.hpp"

#include "ircd.hpp"
#include "ircsplit.hpp"

static const uint8_t REGIS_DEAD = 1;
static const uint8_t REGIS_ALIV = 1;
static const uint8_t REGIS_SERV = 2;
static const uint8_t REGIS_PASS = 4;

Server::Server(sindex_t idx, IrcServer& srv)
  : self(idx), regis(REGIS_DEAD),
    token_(0), near_link_(0), hops_(0),
    boot_time_(0), link_time_(0),
    server(srv) {}

/// incoming server
void Server::connect(Connection conn)
{
  this->regis      = REGIS_ALIV;
  this->near_link_ = server.server_id();
  this->hops_      = 0;
  this->link_time_ = server.create_timestamp();
  this->conn       = conn;
  this->remote_links.clear();
  setup_dg();
}
/// outgoing server
void Server::connect(Connection conn, std::string name, std::string pass)
{
  this->regis      = REGIS_ALIV;
  this->near_link_ = server.server_id();
  this->hops_      = 0;
  this->link_time_ = server.create_timestamp();
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
  [ircd = &server, id = get_id()] (auto buf) {
    auto& srv = ircd->servers.get(id);
    srv.readq.read(buf->data(), buf->size(), {srv, &Server::split_message});
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
    if (msg.size() > 1) {
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
  printf("try_auth(): regis=%u\n", regis);
  if (is_regged()) {
    /// validate server
    printf("validating...\n");
    if (server.accept_remote_server(this->sname, this->spass) == false)
    {
      printf("Disconnected server %s p=%s\n", sname.c_str(), spass.c_str());
      squit("Unknown server");
      return;
    }
    /// enter netburst mode
    server.begin_netburst(*this);
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
    conn->write("SQUIT :" + reason + "\r\n");
    conn->close();
    // only registered servers have users and whatnot
    if (is_regged())
    {
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
  }
  else
  {
    /// remove remote clients on this server
    server.kill_remote_clients_on(get_id(), reason);
  }
  // disable server
  this->regis = REGIS_DEAD;
  server.servers.free(*this);
}
