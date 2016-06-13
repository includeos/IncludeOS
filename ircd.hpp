#pragma once
#include <cstdint>
#include <vector>

#include "client.hpp"
#include "channel.hpp"

#define NO_SUCH_CLIENT    UINT16_MAX
#define NO_SUCH_CHANNEL   UINT16_MAX

class IrcServer {
public:
  using Connection = net::TCP::Connection_ptr;
  using Network    = net::Inet4<VirtioNet>;
  typedef Channel::index_t chindex_t;
  typedef Client::index_t  uindex_t;
  
  IrcServer(Network& inet, uint16_t port, const std::string& name);
  
  inline std::string name() const {
    return server_name;
  }
  inline Client& get_client(size_t idx) {
    return clients.at(idx);
  }
  inline Channel& get_channel(size_t idx) {
    return channels.at(idx);
  }
  
  uindex_t  user_by_name(const std::string&) const;
  chindex_t channel_by_name(const std::string&) const;
  
  // send message to all users visible to user
  void user_bcast(uindex_t user, const std::string&);
  // send message to all users visible to user, except user
  void user_bcast_butone(uindex_t user, const std::string&);
  // send message to all users in a channel
  void chan_bcast(chindex_t idx, const std::string&);
  
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
  
};
