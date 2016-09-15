#include "client.hpp"
#include "ircd.hpp"
#include "tokens.hpp"
#include "modes.hpp"

void Client::send_motd()
{
  const char* prev = server.get_motd();
  const char* next = strchr(prev, '\n');
  
  send(RPL_MOTDSTART, ":- " + server.name() + " Message of the day - ");
  char buffer[1024];
  while (next != nullptr)
  {
    int len = snprintf(buffer, sizeof(buffer),
        ":%s 372 %s :%.*s\r\n",
        server.name().c_str(), nick().c_str(), next - prev, prev);
    send_raw(buffer, len);
    prev = next + 1;
    next = strchr(prev, '\n');
  }
  int len = snprintf(buffer, sizeof(buffer),
      ":%s 372 %s :%s\r\n",
      server.name().c_str(), nick().c_str(), prev);
  send_raw(buffer, len);
  send(RPL_ENDOFMOTD, ":End of MOTD command");
}

void Client::send_lusers()
{
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
