#include <service>

void Service::start()
{
  extern void create_network_device(int N, const char* route, const char* ip);
  create_network_device(0, "10.0.10.0/24", "10.0.0.1");
  create_network_device(1, "10.0.20.0/24", "10.0.0.1");

  extern void register_plugin_nacl();
  register_plugin_nacl();
}
