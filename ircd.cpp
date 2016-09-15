#include "ircd.hpp"
#include "tokens.hpp"
#include <algorithm>
#include <set>
#include <debug>
#include <timers>

#include <kernel/syscalls.hpp>

IrcServer::IrcServer(
    Network& inet_, 
    uint16_t port, 
    const std::string& name, 
    const std::string& netw, 
    const motd_func_t& mfunc)
    : inet(inet_), server_name(name), server_network(netw), motd_func(mfunc)
{
  // initialize lookup tables
  extern void transform_init();
  transform_init();
  
  // timeout for clients and servers
  using namespace std::chrono;
  Timers::periodic(10s, 5s, 
      delegate<void(uint32_t)>::from<IrcServer, &IrcServer::timeout_handler>(this));
  
  // server listener (although IRC servers usually have many ports open)
  auto& tcp = inet.tcp();
  auto& server_port = tcp.bind(port);
  server_port.on_connect(
  [this] (auto csock)
  {
    // one more client in total
    inc_counter(STAT_TOTAL_CONNS);
    inc_counter(STAT_TOTAL_USERS);
    inc_counter(STAT_LOCAL_USERS);
    // possibly set new max users connected
    if (get_counter(STAT_MAX_USERS) < get_counter(STAT_TOTAL_USERS))
      set_counter(STAT_MAX_USERS, get_counter(STAT_LOCAL_USERS));
    
    // in case splitter is bad
    SET_CRASH_CONTEXT("server_port.on_connect(): %s", 
          csock->remote().to_string().c_str());
    
    debug("*** Received connection from %s\n",
          csock->remote().to_string().c_str());

    /// create client ///
    size_t clindex = new_client();
    auto& client = clients[clindex];
    // make sure crucial fields are reset properly
    client.reset_to(csock);

    // set up callbacks
    csock->on_read(128,
    [this, clindex] (net::tcp::buffer_t buffer, size_t bytes)
    {
      auto& client = clients[clindex];
      client.read(buffer.get(), bytes);
    });

    csock->on_close(
    [this, clindex] {
      // for the case where the client has not voluntarily quit,
      auto& client = clients[clindex];
      //assert(client.is_alive());
      if (UNLIKELY(!client.is_alive())) return;
      // tell everyone that he just disconnected
      char buff[128];
      int len = snprintf(buff, sizeof(buff),
                ":%s QUIT :%s\r\n", client.nickuserhost().c_str(), "Connection closed");
      client.handle_quit(buff, len);
      // force-free resources
      client.disable();
    });
  });
  
  // set timestamp for when the server was started
  this->created_ts = create_timestamp();
  this->created_string = std::string(ctime(&created_ts));
  this->cheapstamp = this->created_ts;
}

size_t IrcServer::new_client() {
  // use prev dead client
  if (!free_clients.empty()) {
    size_t idx = free_clients.back();
    free_clients.pop_back();
    return idx;
  }
  // create new client
  clients.emplace_back(clients.size(), *this);
  return clients.size()-1;
}
size_t IrcServer::new_channel() {
  // use prev dead channel
  if (!free_channels.empty()) {
    size_t idx = free_channels.back();
    free_channels.pop_back();
    return idx;
  }
  // create new channel
  channels.emplace_back(channels.size(), *this);
  return channels.size()-1;
}

#include <kernel/syscalls.hpp>
void IrcServer::free_client(Client& client)
{
  // give back the client id
  free_clients.push_back(client.get_id());
  // give back nickname, if any
  if (!client.nick().empty())
      erase_nickname(client.nick());
  // one less client in total on server
  dec_counter(STAT_TOTAL_USERS);
  dec_counter(STAT_LOCAL_USERS);
}

IrcServer::uindex_t IrcServer::user_by_name(const std::string& name) const
{
  auto it = h_users.find(name);
  if (it != h_users.end()) return it->second;
  return NO_SUCH_CHANNEL;
}
IrcServer::chindex_t IrcServer::channel_by_name(const std::string& name) const
{
  auto it = h_channels.find(name);
  if (it != h_channels.end()) return it->second;
  return NO_SUCH_CHANNEL;
}

IrcServer::chindex_t IrcServer::create_channel(const std::string& name)
{
  auto ch = new_channel();
  hash_channel(name, ch);
  get_channel(ch).reset(name);
  inc_counter(STAT_CHANNELS);
  return ch;
}
void IrcServer::free_channel(Channel& ch)
{
  // give back channel id
  free_channels.push_back(ch.get_id());
  // give back channel name
  erase_channel(ch.name());
  // less channels on server/network
  dec_counter(STAT_CHANNELS);
}

void IrcServer::user_bcast(uindex_t idx, const std::string& from, uint16_t tk, const std::string& msg)
{
  char buffer[256];
  int len = snprintf(buffer, sizeof(buffer),
            ":%s %03u %s", from.c_str(), tk, msg.c_str());
  
  user_bcast(idx, buffer, len);
}
void IrcServer::user_bcast(uindex_t idx, const char* buffer, size_t len)
{
  std::set<uindex_t> uset;
  // add user
  uset.insert(idx);
  // for each channel user is in
  for (auto ch : get_client(idx).channels())
  {
    // insert all users from channel into set
    for (auto cl : get_channel(ch).clients())
        uset.insert(cl);
  }
  // broadcast message
  for (auto cl : uset)
      get_client(cl).send_raw(buffer, len);
}

void IrcServer::user_bcast_butone(uindex_t idx, const std::string& from, uint16_t tk, const std::string& msg)
{
  char buffer[256];
  int len = snprintf(buffer, sizeof(buffer),
            ":%s %03u %s", from.c_str(), tk, msg.c_str());
  
  user_bcast_butone(idx, buffer, len);
}
void IrcServer::user_bcast_butone(uindex_t idx, const char* buffer, size_t len)
{
  std::set<uindex_t> uset;
  // for each channel user is in
  for (auto ch : get_client(idx).channels())
  {
    // insert all users from channel into set
    for (auto cl : get_channel(ch).clients())
        uset.insert(cl);
  }
  // make sure user is not included
  uset.erase(idx);
  // broadcast message
  for (auto cl : uset)
      get_client(cl).send_raw(buffer, len);
}
