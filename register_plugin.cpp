
#include "ws_uplink.hpp"
#include "common.hpp"

namespace uplink {

static std::unique_ptr<WS_uplink> uplink{nullptr};

void setup_uplink()
{
  MYINFO("Setting up WS uplink");

  net::Inet4::ifconfig<0>(5.0,
    [] (bool timeout)
  {
    if (timeout) {
      // static IP in case DHCP fails
      net::Inet4::stack<0>().network_config(
        { 10,0,0,42 },     // IP
        { 255,255,255,0 }, // Netmask
        { 10,0,0,1 },      // Gateway
        { 10,0,0,1 });     // DNS
    }

    uplink = std::make_unique<WS_uplink>(net::Inet4::stack<0>());
  });
}

}

#include <kernel/os.hpp>
__attribute__((constructor))
void register_plugin_uplink(){
  OS::register_plugin(uplink::setup_uplink, "Uplink");
}