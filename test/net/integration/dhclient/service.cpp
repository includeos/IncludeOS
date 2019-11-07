
//#define DEBUG // Debug supression

#include <os>
#include <list>
#include <net/interfaces>

using namespace net;

void Service::start(const std::string&)
{
  static auto& inet = Interfaces::get(0);
  inet.negotiate_dhcp(10.0, [](bool timeout) {
      if (timeout)
        os::panic("DHCP timed out");

      INFO("DHCP test", "Got IP from DHCP");
      printf("%s\n", inet.ip_addr().str().c_str());
    });
  INFO("DHCP test", "Waiting for DHCP response\n");
}
