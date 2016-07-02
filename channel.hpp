#pragma once
#include <deque>
#include <string>
#include <unordered_set>
#include "modes.hpp"

class IrcServer;
class Client;

class Channel
{
public:
  typedef uint16_t index_t;
  using ClientList = std::deque<index_t>;
  
  Channel(index_t self, IrcServer& sref);
  
  bool alive() const {
    return !clients_.empty();
  }
  index_t get_id() const {
    return self;
  }
  const std::string& name() const {
    return cname;
  }
  size_t size() const {
    return clients_.size();
  }
  
  const ClientList& clients() {
    return clients_;
  }
  
  std::string listed_name(index_t cid) const;
  
  
  bool add(index_t);
  bool remove(index_t);
  
  // the entire join sequence for a client
  bool join(index_t, const std::string& key = "");
  
  bool is_banned(index_t) const {
    return false;
  }
  bool is_excepted(index_t) const {
    return false;
  }
  
  bool is_chanop(index_t cid) const;
  bool is_voiced(index_t cid) const;
  
  void send_mode(Client&);
  void send_topic(Client&);
  void send_names(Client&);
  
  static bool is_channel_identifier(char c) {
    static std::string LUT = "&#+!";
    return LUT.find_first_of(c) != std::string::npos;
  }
  
private:
  index_t     self;
  std::string cmodes;
  uint32_t    ctimestamp;
  std::string cname;
  std::string ctopic;
  std::string ctopic_by;
  uint32_t    ctopic_ts;
  std::string ckey;
  uint16_t    climit;
  IrcServer&  server;
  ClientList  clients_;
  std::unordered_set<index_t> chanops;
  std::unordered_set<index_t> voices;
};
