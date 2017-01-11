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
    auto& client = clients.create(*this);
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
  for (auto ch : clients.get(idx).channels())
  {
    // insert all users from channel into set
    for (auto cl : channels.get(ch).clients())
        uset.insert(cl);
  }
  // broadcast message
  for (auto cl : uset)
      clients.get(cl).send_buffer(netbuff, len);
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
      clients.get(cl).send_buffer(netbuff, len);
}

bool IrcServer::accept_remote_server(const std::string& name, const std::string& pass) const noexcept
{
  for (auto& srv : remote_server_list)
  {
    if (srv.sname == name && srv.spass == pass) return true;
  }
  return false;
}
