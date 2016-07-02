#pragma once

#include <deque>
#include <string>
#include "modes.hpp"

class IrcServer;

class Channel
{
public:
  typedef uint16_t index_t;
  using ClientList = std::deque<index_t>;
  
  Channel(index_t self, IrcServer& sref);
  
  bool alive() const {
    return !clientlist.empty();
  }
  index_t get_id() const {
    return self;
  }
  const std::string& name() const {
    return cname;
  }
  size_t size() const {
    return clientlist.size();
  }
  
  const ClientList& clients() {
    return clientlist;
  }
  
  bool add(index_t);
  bool remove(index_t);
  
  static bool is_channel_identifier(char c) {
    static std::string LUT = "&#+!";
    return LUT.find_first_of(c) != std::string::npos;
  }
  
private:
  index_t     self;
  uint16_t    cmodes;
  uint32_t    ctimestamp;
  std::string cname;
  IrcServer&  server;
  ClientList  clientlist;
};
