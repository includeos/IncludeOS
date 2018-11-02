#include "ircd.hpp"
#include "tokens.hpp"
#include <algorithm>
#include <info>
#include <set>
#include <timers>

#include <kernel/syscalls.hpp>

void IrcServer::init()
{
  Client::init();
  Server::init();
  // initialize lookup tables
  extern void transform_init();
  transform_init();
}

IrcServer::IrcServer(
    Network& client_inet,
    const uint16_t cl_port,
    Network& server_inet,
    const uint16_t sv_port,
    const uint16_t Id,
    const std::string& name,
    const std::string& netw)
    : cli_inet(client_inet),
      srv_inet(server_inet),
      srv_id(Id), server_name(name), network_name(netw)
{
  INFO("IRC", "Starting %s on %s", name.c_str(), netw.c_str());
  IrcServer::init();

  // client listener (although IRC servers usually have many ports open)
  client_stack().tcp().listen(cl_port,
  [this] (auto csock)
  {
    // one more connection in total
    inc_counter(STAT_TOTAL_CONNS);

    // in case splitter is bad
    SET_CRASH_CONTEXT("client_port.on_connect(): %s",
          csock->remote().to_string().c_str());

    debug("*** Received connection from %s\n",
          csock->remote().to_string().c_str());

    /// create client ///
    auto& client = clients.create(*this);
    // make sure crucial fields are reset properly
    client.reset_to(csock);
  });
  INFO2("Accepting clients on %s port %u",
        client_stack().ifname().c_str(), cl_port);

  // server listener
  server_stack().tcp().listen(sv_port,
  [this] (auto ssock)
  {
    // one more connection in total
    inc_counter(STAT_TOTAL_CONNS);

    // in case splitter is bad
    SET_CRASH_CONTEXT("server_port.on_connect(): %s",
          ssock->remote().to_string().c_str());

    printf("*** Received server connection from %s\n",
          ssock->remote().to_string().c_str());

    /// create server ///
    auto& srv = servers.create(*this);
    srv.connect(ssock);
  });
  INFO2("Accepting servers on %s port %u",
        server_stack().ifname().c_str(), sv_port);

#ifndef USERSPACE_LINUX
  /// LiveUpdate ///
  const bool live_updated = this->init_liveupdate();
  if (live_updated == false)
  {
    // set timestamp for when the server was started
    this->created_ts = create_timestamp();
    char str[100];
    int len = strftime(str, sizeof(str), "%A %c", localtime(&created_ts));
    this->created_string = std::string(str, len);
    this->cheapstamp = this->created_ts;
  }
#else
  const bool live_updated = false;
#endif
  INFO2("Server started on %s", created_string.c_str());
  INFO2("Version " IRC_SERVER_VERSION);
  INFO("IRC", "Server open");
  if (live_updated) {
    this->lnotice(this->name(), "Server was just live updated!");
  }
}

void IrcServer::new_registered_client()
{
  inc_counter(STAT_TOTAL_USERS);
  inc_counter(STAT_LOCAL_USERS);
  // possibly set new max users connected
  if (get_counter(STAT_MAX_USERS) < get_counter(STAT_TOTAL_USERS))
      set_counter(STAT_MAX_USERS, get_counter(STAT_LOCAL_USERS));
}
void IrcServer::free_client(Client& client)
{
  // one less client in total on server
  if (client.is_reg()) {
    dec_counter(STAT_TOTAL_USERS);
    dec_counter(STAT_LOCAL_USERS);
  }
  // free from perf array
  clients.free(client);
}

chindex_t IrcServer::create_channel(const std::string& name)
{
  auto& channel = channels.create(*this, name);
  channel.reset(name);
  inc_counter(STAT_CHANNELS);
  return channel.get_id();
}
void IrcServer::free_channel(Channel& ch)
{
  // give back channel
  channels.free(ch);
  // less channels on server/network
  dec_counter(STAT_CHANNELS);
}

void IrcServer::lnotice(const std::string& src, const std::string& msg)
{
  for (size_t id = 0; id < clients.size(); id++)
  {
    auto& cl = clients.get(id);
    if (cl.is_reg()) {
      cl.send_from(src, "NOTICE " + cl.nickuserhost() + " :" + msg);
    }
  }
}

void IrcServer::user_bcast(clindex_t idx, const std::string& from, uint16_t tk, const std::string& msg)
{
  char buffer[256];
  int len = snprintf(buffer, sizeof(buffer),
            ":%s %03u %s", from.c_str(), tk, msg.c_str());

  user_bcast(idx, buffer, len);
}
void IrcServer::user_bcast(clindex_t idx, const char* buffer, size_t len)
{
  // we might save some memory by trying to use a shared buffer
  auto netbuff = std::make_shared<std::vector<uint8_t>> (buffer, buffer + len);

  std::set<clindex_t> uset;
  // add user
  uset.insert(idx);
  // for each channel user is in
  for (auto ch : clients.get(idx).channels())
  {
    // insert all users from channel into set
    for (auto cl : channels.get(ch).clients())
        uset.insert(cl);
  }
  // broadcast message
  for (auto cl : uset)
      clients.get(cl).send_buffer(netbuff);
}

void IrcServer::user_bcast_butone(clindex_t idx, const std::string& from, uint16_t tk, const std::string& msg)
{
  char buffer[256];
  int len = snprintf(buffer, sizeof(buffer),
            ":%s %03u %s", from.c_str(), tk, msg.c_str());

  user_bcast_butone(idx, buffer, len);
}
void IrcServer::user_bcast_butone(clindex_t idx, const char* buffer, size_t len)
{
  // we might save some memory by trying to use a shared buffer
  auto netbuff = std::make_shared<std::vector<uint8_t>> (buffer, buffer + len);

  std::set<clindex_t> uset;
  // for each channel user is in
  for (auto ch : clients.get(idx).channels())
  {
    // insert all users from channel into set
    for (auto cl : channels.get(ch).clients())
        uset.insert(cl);
  }
  // make sure user is not included
  uset.erase(idx);
  // broadcast message
  for (auto cl : uset)
      clients.get(cl).send_buffer(netbuff);
}

bool IrcServer::accept_remote_server(const std::string& name, const std::string& pass) const noexcept
{
  for (auto& srv : remote_server_list)
  {
    if (srv.sname == name && srv.spass == pass) return true;
  }
  return false;
}
void IrcServer::call_remote_servers()
{
  for (auto& remote : remote_server_list)
  {
    // check if we have a server by that name
    auto id = servers.find(remote.sname);
    if (id == NO_SUCH_SERVER) {
      // try to connect to it
      printf("*** Attempting server connection to %s [%s:%u]\n",
            remote.sname.c_str(),
            remote.address.to_string().c_str(),
            remote.port);
      auto conn = server_stack().tcp().connect({remote.address, remote.port});
      auto& srv = servers.create(*this, remote.sname);
      srv.connect(conn, remote.sname, remote.spass);
    }
  }
}
void IrcServer::kill_remote_clients_on(sindex_t srv, const std::string& reason)
{
  for (clindex_t id = 0; id < clients.size(); id++)
  {
    auto& cl = clients.get(id);
    if (cl.is_alive() && cl.get_server_id() == srv)
    {
      cl.kill(false, reason);
    }
  }
}

void IrcServer::sbcast(const std::string& msg)
{
  for (size_t id = 0; id < servers.size(); id++)
  {
    // send message to all local servers
    auto& srv = servers.get(id);
    if (srv.is_regged() && srv.is_local()) {
      srv.send(msg);
    }
  }
}
void IrcServer::sbcast_butone(sindex_t origin, const std::string& msg)
{
  for (sindex_t id = 0; id < (sindex_t) servers.size(); id++)
  {
    if (id == origin) continue;
    // send message to all local servers
    auto& srv = servers.get(id);
    if (srv.is_regged() && srv.is_local()) {
      srv.send(msg);
    }
  }
}

void IrcServer::begin_netburst(Server& target)
{
  //broadcast(
  //this->netburst = true;
  /// send my servers
  for (size_t id = 0; id < servers.size(); id++)
  {
    auto& srv = servers.get(id);
    if (srv.is_regged()) {
      /// [server] SERVER [name] [hops] [boot_ts] [link_ts] [proto] [token] 0 :[desc]
      target.send(std::string(1, srv.nl_token()) + " S " + srv.name() +
            " " + std::to_string(srv.hop_count()) +
            " " + std::to_string(srv.boot_ts()) + // protocol:
            " " + std::to_string(srv.link_ts()) + " J10 " +
            " " + std::string(1, srv.token()) +
            " :" + srv.get_desc() + "\r\n");
    }
  }
  /// clients
  for (size_t id = 0; id < clients.size(); id++)
  {
    auto& cl = clients.get(id);
    if (cl.is_reg()) {
      /// [tk] NICK [nick] [hops] [ts] [user] [host] [+modes] [ip] [numeric] :[rname]
      auto& srv = servers.get(cl.get_server_id());
      target.send(std::string(1,
            srv.token()) + " N " + cl.nick() +
            " " + std::to_string(srv.hop_count()) +
            " " + std::to_string(0u) +
            " " + cl.user() +
            " " + cl.host() +
            " " + cl.mode_string() +
            " " + cl.ip_addr() +
            " " + cl.token() +
            " :" + cl.realname() + "\r\n");
    }
  }
  /// channel bursts
  for (size_t id = 0; id < channels.size(); id++)
  {
    auto& chan = channels.get(id);
    if (chan.is_alive()) {
      /// with topic:
      if (chan.has_topic()) {
        /// [tk] BURST [name] [ts] [+modes] [user] ... :[bans]
        target.send(std::string(1,
          token()) + " B " + chan.name() +
          " " + std::to_string(chan.created()) +
          " " + chan.mode_string() + "\r\n");
      }
      else {
        /// CHANNEL [name] [+modes] [ts]
        target.send("C " + chan.name() + " " + chan.mode_string() + "\r\n");
      }
    }
  }
  /// end of burst
  target.send("EB\r\n");
}
