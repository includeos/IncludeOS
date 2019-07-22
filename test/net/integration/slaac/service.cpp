
//#define DEBUG // Debug supression

#include <os>
#include <net/interfaces>

using namespace net;

void Service::start(const std::string&)
{
  static auto& inet = net::Interfaces::get(0);
  inet.autoconf_v6(1, [](bool completed) {
    if (!completed) {
       os::panic("Auto-configuration of IP address failed");
    }
    INFO("Slaac test", "Got IP from Auto-configuration");
    printf("%s\n", inet.ip6_addr().str().c_str());
  });
  INFO("Slaac test", "Waiting for Auto-configuration");
}
