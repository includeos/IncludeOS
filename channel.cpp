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

bool Channel::add(index_t id)
{
  for (index_t i = 0; i < this->size(); i++) {
    if (unlikely(clients_[i] == id))
        return false;
  }
  clients_.push_back(id);
  return true;
}
bool Channel::remove(index_t id)
{
  for (index_t i = 0; i < this->size(); i++) {
    if (unlikely(clients_[i] == id)) {
      clients_.erase(clients_.begin() + i);
      return true;
    }
  }
  return false;
}

bool Channel::join(index_t cid, const std::string& key)
{
  auto& client = server.get_client(cid);
  
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
  if (is_banned(cid) && !is_excepted(cid))
  {
    client.send(ERR_BANNEDFROMCHAN, name() + " :Cannot join channel (+b)");
    return false;
  }
  /// JOINED ///
  // add user to channel
  bool new_channel = clients_.empty();
  if (!add(cid)) {
    // already in channel
    return false;
  }
  // broadcast to channel that the user joined
  server.chan_bcast(get_id(), ":" + client.nickuserhost() + " JOIN " + name());
  // send current channel modes
  if (new_channel)
    // server creates new channel by setting modes
    client.send("MODE " + name() + " +" + this->cmodes);
  else
    send_mode(client);
  // send current topic
  send_topic(client);
  // send userlist to client
  send_names(client);
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
