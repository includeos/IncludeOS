#pragma once
#include <deque>
#include <string>
#include <unordered_set>

#include "common.hpp"
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
  // reset to reuse in other fashion
  void reset(const std::string& new_name);
  
  const ClientList& clients() {
    return clients_;
  }
  
  std::string listed_name(index_t cid) const;
  
  
  bool    add(index_t);
  index_t find(index_t);
  
  // silently remove client from all channel lists
  bool remove(index_t cl)
  {
    auto idx = find(cl);
    if (idx != NO_SUCH_CLIENT)
    {
      clients_.erase(clients_.begin() + idx);
      voices.erase(cl);
      chanops.erase(cl);
    }
    return idx != NO_SUCH_CLIENT;
  }
  
  // the entire JOIN sequence for a client
  bool join(Client&, const std::string& key = "");
  // and the PART command
  bool part(Client&, const std::string& msg = "");
  // set new channel topic (and timestamp it)
  void set_topic(Client&, const std::string&);
  
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
  
  // send message to all users in a channel
  void bcast(const std::string& from, uint16_t tk, const std::string& msg);
  void bcast_butone(index_t src, const std::string&);
  void bcast(const std::string&);
  
private:
  std::string mode_string() const;
  void revalidate_channel();
  
  index_t     self;
  uint16_t    cmodes;
  long        create_ts;
  std::string cname;
  std::string ctopic;
  std::string ctopic_by;
  long        ctopic_ts;
  std::string ckey;
  uint16_t    climit;
  IrcServer&  server;
  ClientList  clients_;
  std::unordered_set<index_t> chanops;
  std::unordered_set<index_t> voices;
};
