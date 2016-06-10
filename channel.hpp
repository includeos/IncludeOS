#pragma once

#include <deque>
#include <string>

class IrcServer;

class Channel
{
public:
  typedef uint16_t index_t;
  
  Channel(index_t self, IrcServer& sref);
  
  bool alive() const {
    return !clients.empty();
  }
  index_t get_id() const {
    return self;
  }
  
  bool add(index_t);
  bool remove(index_t);
  
private:
  index_t     self;
  uint16_t    cmode;
  uint32_t    ctimestamp;
  std::string cname;
  IrcServer&  server;
  std::deque<index_t> clients;
};
