#include "client.hpp"
#include "ircd.hpp"
#include "tokens.hpp"
#include "modes.hpp"

void Client::send_motd()
{
  send(RPL_MOTDSTART, ":- " + server.name() + " Message of the day - ");
  const auto& motd = server.get_motd();
  
  for (const auto& line : motd)
    send(RPL_MOTD, ":" + line);
  
  send(RPL_ENDOFMOTD, ":End of MOTD command");
}

void Client::send_lusers()
{
  send(RPL_LUSERCLIENT, ":There are " + std::to_string(server.get_counter(STAT_TOTAL_USERS)) +
                        " and 0 services on 1 servers");
  send(RPL_LUSEROP,       std::to_string(server.get_counter(STAT_OPERATORS)) + " :operator(s) online");
  send(RPL_LUSERCHANNELS, std::to_string(server.get_counter(STAT_CHANNELS)) + " :channels formed");
  send(RPL_LUSERME, ":I have " + std::to_string(server.get_counter(STAT_LOCAL_USERS)) + " clients and 1 servers");
  
  std::string mu = std::to_string(server.get_counter(STAT_MAX_USERS));
  std::string tc = std::to_string(server.get_counter(STAT_TOTAL_CONNS));
  send(250, "Highest connection count: " + mu + " (" + mu + " clients) (" + tc + " connections received)");
}

void Client::send_modes()
{
  send_raw(":" + nickuserhost() + " MODE " + nick() + " :+" + this->mode_string());
}

void Client::send_quit(const std::string& reason)
{
  std::string text = ":" + nickuserhost() + " " + TK_QUIT + " :" + reason;
  
  /// inform others about disconnect
  server.user_bcast(get_id(), text);
  // remove client from various lists
  for (size_t idx : channels()) {
    server.get_channel(idx).remove(get_id());
  }
  // disable self
  disable();
  // close connection after write
  text += "\r\n";
  conn->write(text.data(), text.size(),
  [hest = conn] (size_t) {
    hest->close();
  });
}
