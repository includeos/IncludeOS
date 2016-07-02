#include "channel.hpp"

#include "ircd.hpp"
#include "client.hpp"

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

Channel::Channel(index_t idx, IrcServer& sref)
  : self(idx), server(sref)
{
  cmodes = default_channel_modes();
}

bool Channel::add(index_t id) {
  for (index_t i = 0; i < this->size(); i++) {
    if (unlikely(clientlist[i] == id))
        return false;
  }
  clientlist.push_back(id);
  return true;
}
bool Channel::remove(index_t id) {
  for (Client::index_t i = 0; i < this->size(); i++) {
    if (unlikely(clientlist[i] == id)) {
      clientlist.erase(clientlist.begin() + i);
      return true;
    }
  }
  return false;
}
