#pragma once
#include <cstdint>
#include <vector>
#include <functional>

#include "client.hpp"
#include "channel.hpp"

#define NO_SUCH_CLIENT    UINT16_MAX
#define NO_SUCH_CHANNEL   UINT16_MAX

class IrcServer {
public:
  using Connection = net::TCP::Connection_ptr;
  using Network    = net::Inet4<VirtioNet>;
  typedef std::function<const std::vector<std::string>&()> motd_func_t;
  typedef Channel::index_t chindex_t;
  typedef Client::index_t  uindex_t;
  
  IrcServer(Network& inet, uint16_t port, const std::string& name, motd_func_t);
  
  const std::string& name() const {
    return server_name;
  }
  std::string version() const
  {
    return "v0.1";
  }
  const std::vector<std::string>& get_motd() const
  {
    return motd_func();
  }
  
  inline Client& get_client(size_t idx) {
    return clients.at(idx);
  }
  inline Channel& get_channel(size_t idx) {
    return channels.at(idx);
  }
  uindex_t  user_by_name(const std::string&) const;
  chindex_t channel_by_name(const std::string&) const;
  
  bool is_channel(const std::string& param) {
    if (param.empty()) return false;
    return Channel::is_channel_identifier(param[0]);
  }
  
  // send message to all users visible to user, including user
  void user_bcast(uindex_t user, const std::string&);
  void user_bcast(uindex_t user, const std::string& from, uint16_t tk, const std::string&);
  // send message to all users visible to user, except user
  void user_bcast_butone(uindex_t user, const std::string&);
  void user_bcast_butone(uindex_t user, const std::string& from, uint16_t tk, const std::string&);
  // send message to all users in a channel
  void chan_bcast(chindex_t idx, const std::string&);
  void chan_bcast(chindex_t idx, const std::string& from, uint16_t tk, const std::string&);
  
  // stats
  size_t get_total_clients() const
  {
    return s_clients_tot;
  }
  size_t get_total_ops() const
  {
    return s_ircops;
  }
  size_t get_total_chans() const
  {
    return s_channels;
  }
  
  // server configuration stuff
  uint8_t nick_minlen() const {
    return 1;
  }
  uint8_t nick_maxlen() const {
    return 9;
  }
  uint8_t chan_minlen() const {
    return 1;
  }
  uint8_t chan_maxlen() const {
    return 16;
  }
  uint8_t chan_max() const {
    return 8;
  }
  
private:
  size_t free_client();
  size_t free_channel();
  
  Network&    network;
  Connection  server;
  std::string server_name;
  std::vector<Client> clients;
  std::vector<size_t> free_clients;
  std::vector<Channel> channels;
  std::vector<size_t> free_channels;
  
  motd_func_t motd_func;
  
  size_t s_clients_reg;
  size_t s_clients_tot;
  size_t s_ircops;
  size_t s_channels;
};
