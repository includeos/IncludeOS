#include "ircd.hpp"
#include "tokens.hpp"

#include <set>

IrcServer::IrcServer(
    Network& inet, uint16_t port, const std::string& name, motd_func_t mfunc)
    : network(inet), server_name(name), motd_func(mfunc)
{
  auto& tcp = network.tcp();
  auto& server_port = tcp.bind(port);
  server_port.onConnect(
  [this] (auto csock)
  {
    // one more client in total
    inc_counter(STAT_TOTAL_CONNS);
    inc_counter(STAT_TOTAL_USERS);
    inc_counter(STAT_LOCAL_USERS);
    
    printf("*** Received connection from %s\n",
    csock->remote().to_string().c_str());

    /// create client ///
    size_t clindex = free_client();
    auto& client = clients[clindex];
    // make sure crucial fields are reset properly
    client.reset(csock);

    // set up callbacks
    csock->read(1024,
    [this, &client] (net::TCP::buffer_t buffer, size_t bytes)
    {
      client.read(buffer.get(), bytes);
    });

    csock->onDisconnect(
    [this, &client] (auto, std::string reason)
    {
      // one less client in total
      dec_counter(STAT_TOTAL_USERS);
      dec_counter(STAT_LOCAL_USERS);
      
      /// inform others about disconnect
      user_bcast_butone(client.get_id(), 
          ":" + client.nickuserhost() + " " + TK_QUIT + " :" + reason);
      // mark as disabled
      client.disable();
      // remove client from various lists and
      for (size_t idx : client.channels()) {
        get_channel(idx).remove(client.get_id());
      }
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
      get_client(cl).send(message);
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
      get_client(cl).send(message);
}

void IrcServer::chan_bcast(chindex_t ch, const std::string& from, uint16_t tk, const std::string& msg)
{
  chan_bcast(ch, ":" + from + " " + std::to_string(tk) + " " + msg);
}
void IrcServer::chan_bcast(chindex_t ch, const std::string& message)
{
  // broadcast to all users in channel
  for (auto cl : get_channel(ch).clients())
      get_client(cl).send(message);
}
