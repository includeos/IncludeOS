#include "channel.hpp"

#include "ircd.hpp"
#include "client.hpp"
#include "tokens.hpp"
#include <common>

Channel::Channel(index_t idx, IrcServer& sref)
  : self(idx), server(sref)
{
  cmodes = default_channel_modes();
}

void Channel::reset(const std::string& new_name)
{
  cmodes = default_channel_modes();
  cname = new_name;
  ctopic.clear();
  ckey.clear();
  climit = 0;
  clients_.clear();
  clients_.shrink_to_fit();
  chanops.clear();
  voices.clear();
}

bool Channel::add(clindex_t id)
{
  if (std::find(clients_.cbegin(), clients_.cend(), id) == clients_.cend())
  {
    clients_.push_back(id);
    return true;
  }
  return false;
}
size_t Channel::find(clindex_t id)
{
  for (size_t i = 0; i < this->size(); i++) {
    if (UNLIKELY(clients_[i] == id)) {
      return i;
    }
  }
  return NO_SUCH_CHANNEL_INDEX;
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
  // add user to channel
  if (!add(cid)) {
    // already in channel
    return false;
  }
  // broadcast to channel that the user joined
  char buff[128];
  int len = snprintf(buff, sizeof(buff),
            ":%s JOIN %s\r\n", client.nickuserhost().c_str(), name().c_str());
  bcast(buff, len);
  // send current channel modes
  if (new_channel) {
    // set creation timestamp
    create_ts = server.create_timestamp();
    // server creates new channel by setting modes
    len = snprintf(buff, sizeof(buff),
        ":%s MODE %s +%s\r\n",
        server.name().c_str(), name().c_str(), mode_string().c_str());
    client.send_raw(buff, len);
    // client is operator when he creates it
    chanops.insert(cid);
  }
  else {
    send_mode(client);
    // send channel topic (but only if the topic was set)
    if (has_topic())
        send_topic(client);
  }
  // send userlist to client
  send_names(client);
  return true;
}

bool Channel::part(Client& client, const std::string& reason)
{
  auto cid = client.get_id();
  auto found = this->find(cid);
  if (found == NO_SUCH_CHANNEL_INDEX)
  {
    client.send(ERR_NOTONCHANNEL, name() + " :You're not on that channel");
    return false;
  }
  // broadcast that client left the channel
  char buff[128];
  int len = snprintf(buff, sizeof(buff),
            ":%s PART %s :%s\r\n", client.nickuserhost().c_str(), name().c_str(), reason.c_str());
  bcast(buff, len);
  // remove client from channels lists
  chanops.erase(cid);
  voices.erase(cid);
  clients_.erase(clients_.begin() + found);
  return true;
}

void Channel::set_topic(Client& client, const std::string& new_topic)
{
  this->ctopic = new_topic;
  this->ctopic_by = client.nickuserhost();
  this->ctopic_ts = server.create_timestamp();
  // broadcast change
  char buff[256];
  int len = snprintf(buff, sizeof(buff),
            ":%s TOPIC %s :%s\r\n", client.nickuserhost().c_str(), name().c_str(), new_topic.c_str());
  bcast(buff, len);
}

bool Channel::is_chanop(clindex_t cid) const
{
  return chanops.find(cid) != chanops.end();
}
bool Channel::is_voiced(clindex_t cid) const
{
  return voices.find(cid) != voices.end();
}

char Channel::listed_symb(clindex_t cid) const
{
  if (is_chanop(cid))
    return '@';
  else if (is_voiced(cid))
    return '+';
  else
    return 0;
}

std::string Channel::mode_string() const
{
  std::string str;  str.reserve(10);
  for (int i = 0; i < 9; i++) {
    if (cmodes & (1 << i)) str += chanmodes.get()[i];
  }
  return str;
}

void Channel::send_mode(Client& client)
{
  client.send(RPL_CHANNELMODEIS, name() + " +" + mode_string());
  client.send(RPL_CHANNELCREATED, name() + " " + std::to_string(create_ts));
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
  std::string list;
  list.reserve(256);
  int restart = 0;

  for (auto idx : clients_)
  {
    if (restart == 0) {
      restart = 25;
      // flush if not empty
      if (!list.empty()) {
        list.append("\r\n");
        client.send_raw(list.data(), list.size());
        list.clear();
      }
      // restart list
      list.append(":" + server.name());
      list.append(" " + std::to_string(RPL_NAMREPLY) + " ");
      list.append(client.nick() + " = " + this->name() + " :");
    }

    char symb = listed_symb(idx);
    if (symb) list.append(&symb, 1);
    list.append(server.clients.get(idx).nick());
    list.append(" ");
    restart--;
  }
  list.append("\r\n");
  client.send_raw(list.data(), list.size());

  int len = snprintf((char*) list.data(), list.capacity(),
        ":%s %03u %s :End of NAMES list\r\n",
        server.name().c_str(),  RPL_ENDOFNAMES,  name().c_str());
  client.send_raw(list.data(), len);
}

void Channel::bcast(const std::string& from, uint16_t tk, const std::string& msg)
{
  char buff[256];
  int len = snprintf(buff, sizeof(buff),
            ":%s %03u %s\r\n", from.c_str(), tk, msg.c_str());
  bcast(buff, len);
}

void Channel::bcast(const char* buff, size_t len)
{
  auto sbuf = net::tcp::construct_buffer (buff, buff + len);

  // broadcast to all users in channel
  for (auto cl : clients()) {
      server.clients.get(cl).send_buffer(sbuf);
  }
}
void Channel::bcast_butone(clindex_t src, const char* buff, size_t len)
{
  auto sbuf = net::tcp::construct_buffer (buff, buff + len);

  // broadcast to all users in channel except source
  for (auto cl : clients())
  if (LIKELY(cl != src)) {
      server.clients.get(cl).send_buffer(sbuf);
  }
}
