#pragma once
#include <cstdint>
#include <vector>
#include <functional>

#include "client.hpp"
#include "channel.hpp"

#define NO_SUCH_CLIENT    UINT16_MAX
#define NO_SUCH_CHANNEL   UINT16_MAX

#define STAT_TOTAL_CONNS           0
#define STAT_TOTAL_USERS           1
#define STAT_LOCAL_USERS           2
#define STAT_REGGED_USERS          3
#define STAT_OPERATORS             4
#define STAT_CHANNELS              6

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
  uindex_t  user_by_name(const std::string& name) const;
  chindex_t channel_by_name(const std::string& name) const;
  
  void hash_nickname(const std::string& old, const std::string& nick, size_t id)
  {
    if (!old.empty()) h_users.erase(old);
    h_users[nick] = id;
  }
  void erase_nickname(const std::string& nick)
  {
    h_users.erase(nick);
  }
  
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
  
  // stats / counters
  void inc_counter(uint8_t counter) {
    statcounters[counter]++;
  }
  void dec_counter(uint8_t counter) {
    statcounters[counter]--;
  }
  size_t get_counter(uint8_t counter) {
    return statcounters[counter];
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
  
  // server callbacks
  motd_func_t motd_func;
  
  // hash table for nicknames, channels etc
  std::map<std::string, size_t> h_users;
  std::map<std::string, size_t> h_channels;
  
  // statistics
  size_t statcounters[8] {0};
};
