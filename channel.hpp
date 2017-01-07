#pragma once
#include <deque>
#include <string>
#include <unordered_set>

#include "common.hpp"
#include "modes.hpp"

class  IrcServer;
class  Client;
namespace liu {
  struct Storage;
  struct Restore;
}

class Channel
{
public:
  typedef uint16_t index_t;
  using ClientList = std::deque<clindex_t>;

  Channel(index_t self, IrcServer& sref);

  bool is_alive() const noexcept {
    return !clients_.empty();
  }
  index_t get_id() const noexcept {
    return self;
  }
  const std::string& name() const noexcept {
    return cname;
  }
  size_t size() const noexcept {
    return clients_.size();
  }
  // reset to reuse in other fashion
  void reset(const std::string& new_name);

  const ClientList& clients() {
    return clients_;
  }

  char listed_symb(clindex_t cid) const;


  bool    add(clindex_t);
  index_t find(clindex_t);

  // silently remove client from all channel lists
  bool remove(clindex_t cl)
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
  
  bool has_topic() const noexcept {
    return !ctopic.empty();
  }
  // set new channel topic (and timestamp it)
  void set_topic(Client&, const std::string&);

  bool is_banned(clindex_t) const noexcept {
    return false;
  }
  bool is_excepted(clindex_t) const noexcept {
    return false;
  }

  bool is_chanop(clindex_t cid) const;
  bool is_voiced(clindex_t cid) const;

  void send_mode(Client&);
  void send_topic(Client&);
  void send_names(Client&);

  static bool is_channel_identifier(char c) {
    static std::string LUT = "&#+!";
    return LUT.find_first_of(c) != std::string::npos;
  }

  // send message to all users in a channel
  void bcast(const char*, size_t);
  void bcast(const std::string& from, uint16_t tk, const std::string& msg);
  void bcast_butone(clindex_t src, const char*, size_t);

  void serialize_to(liu::Storage&);
  void deserialize(liu::Restore&);

private:
  std::string mode_string() const;

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
  std::unordered_set<clindex_t> chanops;
  std::unordered_set<clindex_t> voices;
};
