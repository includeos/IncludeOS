#pragma once
#include <vector>
#include <functional>
#include <net/inet4>
#include <rtc>

#include "common.hpp"
#include "client.hpp"
#include "channel.hpp"
#include "ciless.hpp"

#define STAT_TOTAL_CONNS           0
#define STAT_TOTAL_USERS           1
#define STAT_LOCAL_USERS           2
#define STAT_REGGED_USERS          3
#define STAT_OPERATORS             4
#define STAT_CHANNELS              5
#define STAT_MAX_USERS             6

class IrcServer {
public:
  using Connection = net::tcp::Connection_ptr;
  using Network    = net::Inet<net::IP4>;
  typedef std::function<const std::string&()> motd_func_t;
  
  IrcServer(
      Network& inet, 
      uint16_t port, 
      const std::string& name, 
      const std::string& network, 
      const motd_func_t&);
  
  const std::string& name() const noexcept {
    return server_name;
  }
  const std::string& network() const noexcept {
    return server_network;
  }
  std::string version() const noexcept
  {
    return IRC_SERVER_VERSION;
  }
  const std::string& get_motd()
  {
    return motd_func();
  }
  
  Client& get_client(clindex_t idx) {
    return clients.at(idx);
  }
  void new_registered_client(Client&);
  void free_client(Client&);
  
  Channel& get_channel(chindex_t idx) {
    return channels.at(idx);
  }
  void free_channel(Channel&);
  
  clindex_t user_by_name(const std::string&) const;
  chindex_t channel_by_name(const std::string&) const;
  
  void hash_nickname(const std::string& nick, clindex_t id)
  {
    h_users[nick] = id;
  }
  void erase_nickname(const std::string& nick)
  {
    h_users.erase(nick);
  }
  
  void hash_channel(const std::string& name, chindex_t id)
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
  void user_bcast(clindex_t user, const char* buffer, size_t len);
  void user_bcast(clindex_t user, const std::string& from, uint16_t tk, const std::string&);
  // send message to all users visible to user, except user
  void user_bcast_butone(clindex_t user, const char* buffer, size_t len);
  void user_bcast_butone(clindex_t user, const std::string& from, uint16_t tk, const std::string&);
  
  // create channel on server
  chindex_t create_channel(const std::string& name);
  
  // stats / counters
  void inc_counter(uint8_t counter) {
    statcounters[counter]++;
  }
  void dec_counter(uint8_t counter) {
    statcounters[counter]--;
  }
  int get_counter(uint8_t c) {
    return statcounters[c];
  }
  void set_counter(uint8_t c, int val) {
    statcounters[c] = val;
  }
  
  // server configuration stuff
  constexpr static 
  uint8_t nick_minlen() noexcept {
    return 1;
  }
  constexpr static 
  uint8_t nick_maxlen() noexcept {
    return 9;
  }
  constexpr static 
  uint8_t chan_minlen() noexcept {
    return 1;
  }
  constexpr static 
  uint8_t chan_maxlen() noexcept {
    return 16;
  }
  constexpr static 
  uint8_t chan_max() noexcept {
    return 8;
  }
  constexpr static 
  uint8_t client_maxchans() noexcept {
    return 10;
  }
  
  constexpr static 
  uint16_t readq_max() noexcept {
    return 512;
  }
  
  constexpr static 
  uint16_t ping_timeout() noexcept {
    return 120;
  }
  constexpr static 
  uint16_t short_ping_timeout() noexcept {
    return 20;
  }
  
  size_t clis() const noexcept {
    return clients.size();
  }
  size_t club() const noexcept {
    return clients.capacity();
  }
  
  // create a now() timestamp
  long create_timestamp() const noexcept {
    return RTC::now();
  }
  // imprecise timestamp
  long get_cheapstamp() const noexcept {
    return cheapstamp;
  }
  // date server was created
  const std::string& created() const noexcept {
    return created_string;
  }
  // uptime in seconds
  long uptime() const noexcept {
    return create_timestamp() - this->created_ts;
  }
  
  void print_stuff() {
    int i = 0;
    int hmm = 0;
    for (auto& cl : clients) {
      
      if (cl.getconn()->sendq_size() == 0) continue;
      
      printf("CL[%04d] sendq: %u b sendq rem: %u can send: %d queued: %d b\t",
          i++,
          cl.getconn()->sendq_size(),
          cl.getconn()->sendq_remaining(),
          cl.getconn()->can_send(),
          cl.getconn()->is_queued());
      printf(" %s\n", cl.getconn()->state().to_string().c_str());
      if (cl.getconn()->sendq_size()) hmm++;
    }
    printf("HMM: %d  TOTAL: %u\n", hmm, clients.size());
  }
  
private:
  size_t to_current = 0;
  void   timeout_handler(uint32_t);
  
  clindex_t new_client();
  chindex_t new_channel();
  
  Network&    inet;
  Connection  server;
  std::string server_name;
  std::string server_network;
  std::vector<Client> clients;
  std::vector<clindex_t> free_clients;
  std::vector<Channel> channels;
  std::vector<chindex_t> free_channels;
  
  // server callbacks
  motd_func_t motd_func;
  
  // hash table for nicknames, channels etc
  std::map<std::string, clindex_t, ci_less> h_users;
  std::map<std::string, chindex_t, ci_less> h_channels;
  
  // performance stuff
  long cheapstamp;
  
  // statistics
  std::string created_string;
  long        created_ts;
  int statcounters[8] {0};
};
