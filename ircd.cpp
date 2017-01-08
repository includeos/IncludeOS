#include "ircd.hpp"
#include "tokens.hpp"
#include <algorithm>
#include <set>
#include <debug>
#include <timers>

#include <kernel/syscalls.hpp>

IrcServer::IrcServer(
    Network& inet_, 
    const uint16_t cl_port,
    const uint16_t sv_port,
    const uint16_t Id,
    const std::string& name, 
    const std::string& netw, 
    const motd_func_t& mfunc)
    : inet(inet_), server_name(name), network_name(netw), motd_func(mfunc), id(Id)
{
  // initialize lookup tables
  extern void transform_init();
  transform_init();
  
  // timeout for clients and servers
  using namespace std::chrono;
  Timers::periodic(10s, 5s, 
      delegate<void(uint32_t)>::from<IrcServer, &IrcServer::timeout_handler>(this));
  
  auto& tcp = inet.tcp();
  // client listener (although IRC servers usually have many ports open)
  tcp.bind(cl_port).on_connect(
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
    clindex_t clindex = new_client();
    auto& client = clients[clindex];
    // make sure crucial fields are reset properly
    client.reset_to(csock);
  });
  printf("*** Accepting clients on port %u\n", cl_port);

  // server listener
  tcp.bind(sv_port).on_connect(
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
    // TODO
  });
  printf("*** Accepting servers on port %u\n", sv_port);
  
  // set timestamp for when the server was started
  this->created_ts = create_timestamp();
  this->created_string = std::string(ctime(&created_ts));
  this->cheapstamp = this->created_ts;
}

clindex_t IrcServer::new_client() {
  // use prev dead client
  if (!free_clients.empty()) {
    clindex_t idx = free_clients.back();
    free_clients.pop_back();
    return idx;
  }
  // create new client
  chindex_t idx = clients.size();
  clients.emplace_back(idx, *this);
  return idx;
}
chindex_t IrcServer::new_channel() {
  // use prev dead channel
  if (!free_channels.empty()) {
    chindex_t idx = free_channels.back();
    free_channels.pop_back();
    return idx;
  }
  // create new channel
  chindex_t idx = channels.size();
  channels.emplace_back(idx, *this);
  return idx;
}

clindex_t IrcServer::user_by_name(const std::string& name) const
{
  auto it = h_users.find(name);
  if (it != h_users.end()) return it->second;
  return NO_SUCH_CLIENT;
}
void IrcServer::new_registered_client(Client&)
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
  // give back the client id
  free_clients.push_back(client.get_id());
  // give back nickname, if any
  if (!client.nick().empty())
      erase_nickname(client.nick());
}

chindex_t IrcServer::channel_by_name(const std::string& name) const
{
  auto it = h_channels.find(name);
  if (it != h_channels.end()) return it->second;
  return NO_SUCH_CHANNEL;
}
chindex_t IrcServer::create_channel(const std::string& name)
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
  auto netbuff = std::shared_ptr<uint8_t> (new uint8_t[len]);
  memcpy(netbuff.get(), buffer, len);
  
  std::set<clindex_t> uset;
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
      get_client(cl).send_buffer(netbuff, len);
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
  auto netbuff = std::shared_ptr<uint8_t> (new uint8_t[len]);
  memcpy(netbuff.get(), buffer, len);
  
  std::set<clindex_t> uset;
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
      get_client(cl).send_buffer(netbuff, len);
}
