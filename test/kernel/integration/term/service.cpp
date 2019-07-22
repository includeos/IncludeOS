
#include <service>
#include <cstdio>
#include <net/interfaces>
#include <terminal>

void Service::start()
{
  auto& inet = net::Interfaces::get(0);
  inet.network_config(
     { 10,0,0,63 },      // IP
     { 255,255,255,0 },  // Netmask
     { 10,0,0,1 });      // GW

  // add 'netstat' command
  Terminal::register_program(
     "netstat", {"Print network connections",
  [&inet] (Terminal& term, const std::vector<std::string>&) -> int {
     term.write("%s\r\n", inet.tcp().status().c_str());
     printf("SUCCESS\n");
     return 0;
  }});

  #define SERVICE_TELNET    23
  inet.tcp().listen(SERVICE_TELNET,
  [] (auto client) {
    // create terminal with open TCP connection
    static std::unique_ptr<Terminal> term = nullptr;
    term = std::make_unique<Terminal> (client);
  });
  INFO("TERM", "Connect to terminal with $ telnet %s ",
                inet.ip_addr().str().c_str());
}
