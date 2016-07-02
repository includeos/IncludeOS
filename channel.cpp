#include "channel.hpp"

#include "ircd.hpp"
#include "client.hpp"
#include "tokens.hpp"

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

Channel::Channel(index_t idx, IrcServer& sref)
  : self(idx), server(sref)
{
  cmodes = default_channel_modes();
}

void Channel::reset(const std::string& new_name)
{
  cmodes = default_channel_modes();
  ctimestamp = 0; // FIXME
  cname = new_name;
  ctopic.clear();
  ctopic_by.clear();
  ctopic_ts = 0;
  ckey.clear();
  climit = 0;
  clients_.clear();
  chanops.clear();
  voices.clear();
}

bool Channel::add(index_t id)
{
  for (index_t i = 0; i < this->size(); i++) {
    if (unlikely(clients_[i] == id))
        return false;
  }
  clients_.push_back(id);
  return true;
}
Channel::index_t Channel::find(index_t id)
{
  for (index_t i = 0; i < this->size(); i++) {
    if (unlikely(clients_[i] == id)) {
      return i;
    }
  }
  return NO_SUCH_CLIENT;
}

bool Channel::join(Client& client, const std::string& key)
{
  // verify key, if +k chanmode is set
  if (!ckey.empty())
  if (key != ckey)
  {
    client.send(ERR_BADCHANNELKEY, name() + " :Cannot join channel (+k)");
    return false;
  }
  // verify client joining is within channel user limit
  if (climit != 0)
  if (size() >= climit)
  {
    client.send(ERR_CHANNELISFULL, name() + " :Cannot join channel (+l)");
    return false;
  }
  // verify that we are not banned
  auto cid = client.get_id();
  if (is_banned(cid) && !is_excepted(cid))
  {
    client.send(ERR_BANNEDFROMCHAN, name() + " :Cannot join channel (+b)");
    return false;
  }
  /// JOINED ///
  bool new_channel = clients_.empty();
  // register new channels on server (for hash map)
  if (new_channel) server.hash_channel(name(), get_id());
  // add user to channel
  if (!add(cid)) {
    // already in channel
    return false;
  }
  // broadcast to channel that the user joined
  bcast(":" + client.nickuserhost() + " JOIN " + name());
  // send current channel modes
  if (new_channel) {
    // server creates new channel by setting modes
    client.send("MODE " + name() + " +" + this->cmodes);
    // client is operator when he creates it
    chanops.insert(cid);
  }
  else {
    send_mode(client);
    // send current topic (but only if the channel existed before)
    send_topic(client);
  }
  // send userlist to client
  send_names(client);
  return true;
}

bool Channel::part(Client& client, const std::string& msg)
{
  auto cid = client.get_id();
  index_t found = find(cid);
  if (found == NO_SUCH_CLIENT)
  {
    client.send(ERR_NOTONCHANNEL, name() + " :You're not on that channel");
    return false;
  }
  // broadcast that client left the channel
  bcast(":" + client.nickuserhost() + " PART " + name() + " :" + msg);
  // remove client from channels lists
  chanops.erase(cid);
  voices.erase(cid);
  clients_.erase(clients_.begin() + found);
  // validate the channel, since it could be empty
  revalidate_channel();
  return true;
}

bool Channel::is_chanop(index_t cid) const
{
  return chanops.find(cid) != chanops.end();
}
bool Channel::is_voiced(index_t cid) const
{
  return voices.find(cid) != voices.end();
}

std::string Channel::listed_name(index_t cid) const
{
  if (is_chanop(cid))
    return "@" + server.get_client(cid).nick();
  if (is_voiced(cid))
    return "+" + server.get_client(cid).nick();
  return server.get_client(cid).nick();
}

void Channel::send_mode(Client& client)
{
  client.send(RPL_CHANNELMODEIS, name() + " +" + this->cmodes);
  client.send(RPL_CHANNELCREATED, name() + " 0");
}
void Channel::send_topic(Client& client)
{
  if (ctopic.empty()) {
    client.send(RPL_NOTOPIC, name() + " :No topic is set");
    return;
  }
  client.send(RPL_TOPIC, name() + " :" + this->ctopic);
  client.send(RPL_TOPICBY, this->ctopic_by + " " + std::to_string(this->ctopic_ts));
}
void Channel::send_names(Client& client)
{
  //:irc.colosolutions.net 353 gonzo_ = #testestsetestes :@gonzo_
  for (index_t i = 0; i < this->size(); i++)
      client.send(RPL_NAMREPLY, " = " + name() + " :" + listed_name(clients_[i]));
  
  client.send(RPL_ENDOFNAMES, name() + " :End of NAMES list");
}

void Channel::revalidate_channel()
{
  if (alive() == false)
  {
    printf("channel died: %s\n", name().c_str());
    server.erase_channel(name());
  }
}

void Channel::bcast(const std::string& from, uint16_t tk, const std::string& msg)
{
  bcast(":" + from + " " + std::to_string(tk) + " " + msg);
}
void Channel::bcast(const std::string& message)
{
  // broadcast to all users in channel
  for (auto cl : clients())
      server.get_client(cl).send_raw(message);
}
void Channel::bcast_butone(index_t src, const std::string& message)
{
  // broadcast to all users in channel except source
  for (auto cl : clients())
    if (likely(cl != src))
    server.get_client(cl).send_raw(message);
}
