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
  send_raw(":" + nickuserhost() + " " + TK_MODE + " " + nick() + " +" + this->mode_string());
}

void Client::send_uptime()
{
  static const int DAY = 3600 * 24;
  
  auto uptime = server.uptime();
  int days = uptime / DAY;
  uptime -= days * DAY;
  int hours = uptime / 3600;
  uptime -= hours * 3600;
  int mins = uptime / 60;
  int secs = uptime % 60;
  
  char buffer[128];
  int len = snprintf(buffer, sizeof(buffer),
            ":%s %03u %s :Server has been up %d days %d hours, %d minutes and %d seconds\r\n",
            server.name().c_str(), RPL_STATSUPTIME, nick().c_str(), days, hours, mins, secs);
  send_raw(buffer, len);
}
