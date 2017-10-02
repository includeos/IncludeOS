#include "client.hpp"
#include "ircd.hpp"
#include "tokens.hpp"
#include "modes.hpp"
#include <os>

void Client::send_motd()
{
  char     buffer[2048];
  uint16_t total = 0;
  // motd start
  int len = sprintf(buffer, ":%s 375 %s :- %s Message of the day\r\n",
      server.name().c_str(), nick().c_str(), server.name().c_str());
  total += len;
  // motd contents
  const std::string& motd = server.get_motd();
  size_t prev = 0;
  size_t next = motd.find("\n");

  while (next != motd.npos)
  {
    len = snprintf(buffer + total, sizeof(buffer) - total,
        ":%s 372 %s :%.*s\r\n",
        server.name().c_str(), nick().c_str(), (int) (next-prev), &motd[prev]);
    total += len;
    prev = next + 1;
    next = motd.find("\n", prev);
  }
  // last line of motd
  len = snprintf(buffer + total, sizeof(buffer) - total,
      ":%s 372 %s :%s\r\n",
      server.name().c_str(), nick().c_str(), &motd[prev]);
  total += len;
  // end of motd
  len = snprintf(buffer + total, sizeof(buffer) - total,
      ":%s 376 %s :End of MOTD command\r\n",
      server.name().c_str(), nick().c_str());
  total += len;
  send_raw(buffer, total);
}

void Client::send_lusers()
{
  /*
  :wilhelm.freenode.net 251 gonzo__ :There are 149 users and 81096 invisible on 29 servers
  :wilhelm.freenode.net 252 gonzo__ 34 :IRC Operators online
  :wilhelm.freenode.net 253 gonzo__ 13 :unknown connection(s)
  :wilhelm.freenode.net 254 gonzo__ 47699 :channels formed
  :wilhelm.freenode.net 255 gonzo__ :I have 7508 clients and 1 servers
  :wilhelm.freenode.net 265 gonzo__ 7508 8324 :Current local users 7508, max 8324
  :wilhelm.freenode.net 266 gonzo__ 81245 90995 :Current global users 81245, max 90995
  :wilhelm.freenode.net 250 gonzo__ :Highest connection count: 8325 (8324 clients) (404056 connections received)
  */
  send(RPL_LUSERCLIENT, ":There are " + std::to_string(server.get_counter(STAT_TOTAL_USERS)) +
                        " users and 0 services on 1 servers");
  send(RPL_LUSEROP,       std::to_string(server.get_counter(STAT_OPERATORS)) + " :operator(s) online");
  send(RPL_LUSERCHANNELS, std::to_string(server.get_counter(STAT_CHANNELS)) + " :channels formed");
  send(RPL_LUSERME, ":I have " + std::to_string(server.get_counter(STAT_LOCAL_USERS)) + " clients and 1 servers");

  std::string mu = std::to_string(server.get_counter(STAT_MAX_USERS));
  std::string tc = std::to_string(server.get_counter(STAT_TOTAL_CONNS));
  send(250, "Highest connection count: " + mu + " (" + mu + " clients) (" + tc + " connections received)");
}

void Client::send_modes()
{
  char data[128];
  int len = snprintf(data, sizeof(data),
    ":%s MODE %s +%s\r\n", nickuserhost().c_str(), nick().c_str(), mode_string().c_str());

  send_raw(data, len);
}

void Client::send_stats(const std::string& stat)
{
  char buffer[128];
  int  len;

  if (stat == "u")
  {
    static const int DAY = 3600 * 24;

    auto uptime = server.uptime();
    int days = uptime / DAY;
    uptime -= days * DAY;
    int hours = uptime / 3600;
    uptime -= hours * 3600;
    int mins = uptime / 60;
    int secs = uptime % 60;

    len = snprintf(buffer, sizeof(buffer),
          ":%s %03u %s :Server has been up %d days %d hours, %d minutes and %d seconds\r\n",
          server.name().c_str(), RPL_STATSUPTIME, nick().c_str(), days, hours, mins, secs);
  }
  else if (stat == "m")
  {
    len = snprintf(buffer, sizeof(buffer),
          ":%s %03u %s :Heap usage: %.3f mb\r\n",
          server.name().c_str(), 244, nick().c_str(), OS::heap_usage() / 1024.f / 1024.f);
  }
  else {
    len = snprintf(buffer, sizeof(buffer),
          ":%s %03u %s :No such statistic\r\n",
          server.name().c_str(), 249, nick().c_str());
  }
  send_raw(buffer, len);
}
