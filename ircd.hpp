#pragma once
#include <cstdint>
#include <vector>

#include "client.hpp"
#include "channel.hpp"

class IrcServer {
public:
  using Connection = net::TCP::Connection_ptr;
  using Network    = net::Inet4<VirtioNet>;
  
  IrcServer(Network& inet, uint16_t port, const std::string& name);
  
  std::string name() const {
    return server_name;
  }
  Client& get_client(size_t idx) {
    return clients.at(idx);
  }
  Channel& get_channel(size_t idx) {
    return channels.at(idx);
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
