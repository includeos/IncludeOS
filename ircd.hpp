#pragma once
#include <vector>
#include <functional>

#include "common.hpp"
#include "client.hpp"
#include "channel.hpp"

#define STAT_TOTAL_CONNS           0
#define STAT_TOTAL_USERS           1
#define STAT_LOCAL_USERS           2
#define STAT_REGGED_USERS          3
#define STAT_OPERATORS             4
#define STAT_CHANNELS              5
#define STAT_MAX_USERS             6

class IrcServer {
public:
  using Connection = net::TCP::Connection_ptr;
  using Network    = net::Inet4<VirtioNet>;
  typedef std::function<const std::vector<std::string>&()> motd_func_t;
  typedef Channel::index_t chindex_t;
  typedef Client::index_t  uindex_t;
  
  IrcServer(Network& inet, uint16_t port, const std::string& name, const std::string& netw, motd_func_t);
  
  const std::string& name() const {
    return server_name;
  }
  const std::string& network() const {
    return server_network;
  }
  std::string version() const
  {
    return IRC_SERVER_VERSION;
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
  
  void hash_nickname(const std::string& nick, size_t id)
  {
    h_users[nick] = id;
  }
  void erase_nickname(const std::string& nick)
  {
    h_users.erase(nick);
  }
  
  void hash_channel(const std::string& name, size_t id)
  {
    h_channels[name] = id;
  }
  void erase_channel(const std::string& name)
  {
    h_channels.erase(name);
  }
  
  static bool is_channel(const std::string& param) {
    if (param.empty()) return false;
    return Channel::is_channel_identifier(param[0]);
  }
  
  // send message to all users visible to user, including user
  void user_bcast(uindex_t user, const std::string&);
  void user_bcast(uindex_t user, const std::string& from, uint16_t tk, const std::string&);
  // send message to all users visible to user, except user
  void user_bcast_butone(uindex_t user, const std::string&);
  void user_bcast_butone(uindex_t user, const std::string& from, uint16_t tk, const std::string&);
  
  // create channel on server
  chindex_t create_channel(const std::string& name);
  
  // stats / counters
  void inc_counter(uint8_t counter) {
    statcounters[counter]++;
  }
  void dec_counter(uint8_t counter) {
    statcounters[counter]--;
  }
  size_t get_counter(uint8_t c) {
    return statcounters[c];
  }
  void set_counter(uint8_t c, size_t val) {
    statcounters[c] = val;
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
  uint8_t client_maxchans() const {
    return 10;
  }
  
private:
  size_t free_client();
  size_t free_channel();
  
  Network&    inet;
  Connection  server;
  std::string server_name;
  std::string server_network;
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
