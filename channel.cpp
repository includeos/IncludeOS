#include "channel.hpp"

#include "ircd.hpp"
#include "client.hpp"

Channel::Channel(index_t idx, IrcServer& sref)
  : self(idx), server(sref)
{
  
}

bool Channel::add(index_t id) {
  for (index_t i = 0; i < clients.size(); i++) {
    if (clients[i] == id) {
      return false;
    }
  }
  clients.push_back(id);
  return true;
}
bool Channel::remove(index_t id) {
  for (Client::index_t i = 0; i < clients.size(); i++) {
    if (clients[i] == id) {
      clients.erase(clients.begin() + i);
      return true;
    }
  }
  return false;
}
