#include "ircd.hpp"
#include "tokens.hpp"

#include <set>

IrcServer::IrcServer(
    Network& inet_, 
    uint16_t port, 
    const std::string& name, 
    const std::string& netw, 
    motd_func_t mfunc)
    : inet(inet_), server_name(name), server_network(netw), motd_func(mfunc)
{
  // initialize lookup tables
  extern void transform_init();
  transform_init();
  
  // server listener (although IRC servers usually have many ports open)
  auto& tcp = inet.tcp();
  auto& server_port = tcp.bind(port);
  server_port.onConnect(
  [this] (auto csock)
  {
    // one more client in total
    inc_counter(STAT_TOTAL_CONNS);
    inc_counter(STAT_TOTAL_USERS);
    inc_counter(STAT_LOCAL_USERS);
    // possibly set new max users connected
    if (get_counter(STAT_MAX_USERS) < get_counter(STAT_TOTAL_USERS))
      set_counter(STAT_MAX_USERS, get_counter(STAT_LOCAL_USERS));
    
    printf("*** Received connection from %s\n",
    csock->remote().to_string().c_str());

    /// create client ///
    size_t clindex = free_client();
    auto& client = clients[clindex];
    // make sure crucial fields are reset properly
    client.reset_to(csock);

    // set up callbacks
    csock->read(1024,
    [this, &client] (net::TCP::buffer_t buffer, size_t bytes)
    {
      client.read(buffer.get(), bytes);
    });

    csock->onDisconnect(
    [this, clindex] (auto, std::string reason)
    {
      // one less client in total
      dec_counter(STAT_TOTAL_USERS);
      dec_counter(STAT_LOCAL_USERS);
      
      auto& client = clients[clindex];
      if (client.is_reg())
      {
        /// inform others about disconnect
        user_bcast_butone(clindex, 
            ":" + client.nickuserhost() + " " + TK_QUIT + " :" + reason);
        // remove client from various lists
        for (auto idx : client.channels()) {
          get_channel(idx).remove(clindex);
        }
      }
      // mark as disabled
      client.disable();
      // give back the client id
      free_clients.push_back(clindex);
    });
    
  });
}

size_t IrcServer::free_client() {
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
size_t IrcServer::free_channel() {
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
  auto ch = free_channel();
  get_channel(ch).reset(name);
  return ch;
}

void IrcServer::user_bcast(uindex_t idx, const std::string& from, uint16_t tk, const std::string& msg)
{
  user_bcast(idx, ":" + from + " " + std::to_string(tk) + " " + msg);
}
void IrcServer::user_bcast(uindex_t idx, const std::string& message)
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
      get_client(cl).send_raw(message);
}

void IrcServer::user_bcast_butone(uindex_t idx, const std::string& from, uint16_t tk, const std::string& msg)
{
  user_bcast_butone(idx, ":" + from + " " + std::to_string(tk) + " " + msg);
}
void IrcServer::user_bcast_butone(uindex_t idx, const std::string& message)
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
      get_client(cl).send_raw(message);
}
