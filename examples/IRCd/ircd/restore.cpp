#include <liveupdate>
#include "ircd.hpp"
#include "channel.hpp"
#include "client.hpp"
extern IrcServer* ircd;
using namespace liu;

#define BOOT_TESTING
#ifdef BOOT_TESTING
static std::vector<double> timestamps;
static buffer_t bloberino;
#include <kernel/os.hpp>
#endif

bool IrcServer::init_liveupdate()
{
  // register function that saves IRCd state
  LiveUpdate::register_partition("ircd", {this, &IrcServer::serialize});
  // begin restoring saved data
  return LiveUpdate::resume("ircd", {this, &IrcServer::deserialize});
}

void IrcServer::serialize(Storage& storage, const buffer_t*)
{
  // h_users
  storage.add<size_t> (1, clients.hash_map().size());
  for (auto& hu : clients.hash_map()) {
    storage.add_string     (2, hu.first);
    storage.add<clindex_t> (2, hu.second);
  }

  // h_channels
  storage.add<size_t> (1, channels.hash_map().size());
  for (auto& hu : channels.hash_map()) {
    storage.add_string     (3, hu.first);
    storage.add<chindex_t> (3, hu.second);
  }

  // creation time and timestamp
  storage.add_string(4, created_string);
  storage.add<long> (4, created_ts);

  // stats
  storage.add_buffer(5, statcounters, sizeof(statcounters));

  /// channels
  for (auto& ch : channels) {
    if (ch.is_alive()) {
      ch.serialize_to(storage);
    }
  }

  /// clients
  for (auto& cl : clients) {
    if (cl.is_alive()) {
      cl.serialize_to(storage);
    }
  }

}
void IrcServer::deserialize(Restore& thing)
{
  // we are re-entering this function, so store the counters statically
  chindex_t current_chan = 0;
  clindex_t current_client = 0;
  while (thing.is_end() == false)
  {
    switch (thing.get_id()) {
    case 1: {
      /// server
      size_t count;
      count = thing.as_type<size_t> (); thing.go_next();

      for (size_t i = 0; i < count; i++) {
        auto fir = thing.as_string();           thing.go_next();
        auto sec = thing.as_type<clindex_t> (); thing.go_next();
        clients.hash(fir, sec);
      }

      count = thing.as_type<size_t> (); thing.go_next();

      for (size_t i = 0; i < count; i++) {
        auto fir = thing.as_string();           thing.go_next();
        auto sec = thing.as_type<chindex_t> (); thing.go_next();
        channels.emplace_hash(fir, sec);
      }

      created_string = thing.as_string();       thing.go_next();
      created_ts     = thing.as_type<long> ();  thing.go_next();

      auto buf = thing.as_buffer();  thing.go_next();
      if (buf.size() >= sizeof(statcounters)) {
          memcpy(statcounters, buf.data(), buf.size());
      }
      printf("* Resumed server, next id: %u\n", thing.get_id());
      break;
    }
    case 20: /// channels
      // create free channel indices
      while (channels.size() < thing.as_type<chindex_t> ())
      {
        channels.create_empty(*this);
      }
      thing.go_next();
      {
        // create empty channel
        auto& ch = channels.create_empty(*this);
        // deserialize rest of channel
        ch.deserialize(thing);
        // go to next channel id
        current_chan++;
      }
      break;
    case 50: /// clients
      // create free client indices
      while (clients.size() < thing.as_type<clindex_t> ())
      {
        clients.create_empty(*this);
      }
      thing.go_next();
      {
        // create empty client
        auto& cl = clients.create_empty(*this);
        // deserialize rest of client
        cl.deserialize(thing);
        // assign event handlers for client
        cl.assign_socket_dg();
        // go to next client id
        current_client++;
      }
      break;
    default:
      printf("Unknown ID: %u\n", thing.get_id());
      thing.go_next();
      break;
    }
  } // while (is_end() == false)
}

void Client::serialize_to(Storage& storage)
{
  /*
  clindex_t   self;
  uint8_t     regis;
  uint8_t     bits;
  uint16_t    umodes_;
  IrcServer&  server;
  Connection  conn;
  long        to_stamp;

  std::string nick_;
  std::string user_;
  std::string host_;
  ChannelList channels_;

  std::string readq;
  */
  /// start with index
  storage.add<clindex_t> (50, self);
  // R, B, U
  storage.add_int       (51, regis);
  storage.add_int       (52, server_id);
  storage.add_int       (53, umodes_);
  // Connection
  storage.add_connection(54, conn);
  // timeout ts
  storage.add<long>     (55, to_stamp);
  // N U H
  storage.add_string    (56, nick_);
  storage.add_string    (57, user_);
  storage.add_string    (58, host_);
  // channels
  std::vector<chindex_t> chans;
  for (auto& ch : channels_) chans.push_back(ch);
  storage.add_vector<chindex_t> (59, chans);
  // readq
  storage.add_string(60, readq.get());

}
void Client::deserialize(Restore& thing)
{
  //printf("Deserializing client %u ...", get_id());
  /// NOTE: index already consumed
  // R, B, U
  regis     = thing.as_int(); thing.go_next();
  server_id = thing.as_int(); thing.go_next();
  umodes_   = thing.as_int(); thing.go_next();
  // TCP connection
  conn = thing.as_tcp_connection(server.get_stack().tcp());
  thing.go_next();
  // timeout
  to_stamp = thing.as_type<long> (); thing.go_next();
  // N U H
  nick_ = thing.as_string(); thing.go_next();
  user_ = thing.as_string(); thing.go_next();
  host_ = thing.as_string(); thing.go_next();
  // channels
  auto chans = thing.as_vector<chindex_t> ();
  for (auto& ch : chans) channels_.push_back(ch);
  thing.go_next();
  // readq
  readq.set(thing.as_string()); thing.go_next();
}

void Channel::serialize_to(Storage& storage)
{
  /*
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
  */
  /// start with index
  storage.add<chindex_t> (20, self);

  storage.add<uint16_t> (21, cmodes);
  storage.add<long>     (22, create_ts);
  storage.add_string    (23, cname);
  storage.add_string    (24, ctopic);
  storage.add_string    (25, ctopic_by);
  storage.add<long>     (26, ctopic_ts);
  storage.add_string    (27, ckey);
  storage.add<uint16_t> (28, climit);
  /// clients
  std::vector<clindex_t> cli;
  for (auto& cl : clients_) cli.push_back(cl);
  storage.add_vector<clindex_t> (29, cli);
  /// chanops
  cli.clear();
  for (auto& cl : chanops) cli.push_back(cl);
  storage.add_vector<clindex_t> (30, cli);
  /// voices
  cli.clear();
  for (auto& cl : voices) cli.push_back(cl);
  storage.add_vector<clindex_t> (31, cli);
  /// done ///
}
void Channel::deserialize(Restore& thing)
{
  /// NOTE: index already consumed
  cmodes    = thing.as_type<uint16_t> (); thing.go_next();
  create_ts = thing.as_type<long> (); thing.go_next();
  cname     = thing.as_string();      thing.go_next();
  ctopic    = thing.as_string();      thing.go_next();
  ctopic_by = thing.as_string();      thing.go_next();
  ctopic_ts = thing.as_type<long> (); thing.go_next();
  ckey      = thing.as_string();      thing.go_next();
  climit    = thing.as_type<uint16_t> (); thing.go_next();

  // clients
  auto cli = thing.as_vector<clindex_t> (); thing.go_next();
  for (auto& cl : cli) clients_.push_back(cl);

  // chanops
  cli = thing.as_vector<clindex_t> (); thing.go_next();
  for (auto& cl : cli) chanops.insert(cl);

  // voices
  cli = thing.as_vector<clindex_t> (); thing.go_next();
  for (auto& cl : cli) voices.insert(cl);
  ///
}
