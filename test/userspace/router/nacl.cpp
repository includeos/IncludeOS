#include <iostream>
#include <net/interfaces>
#include <net/ip4/cidr.hpp>
#include <net/router.hpp>
using namespace net;
std::unique_ptr<Router<IP4>> nacl_router_obj;
std::shared_ptr<Conntrack>   nacl_ct_obj;

void register_plugin_nacl()
{
  INFO("NaCl", "Registering NaCl plugin");
  auto &eth0 = Interfaces::get(0);
  eth0.network_config(IP4::addr{10, 0, 0, 42}, IP4::addr{255, 255, 255, 0}, 0);
  auto &eth1 = Interfaces::get(0);
  eth1.network_config(IP4::addr{10, 0, 20, 42}, IP4::addr{255, 255, 255, 0}, 0);
  return;
  // Router
  INFO("NaCl", "Setup routing");
  Router<IP4>::Routing_table routing_table{
      {IP4::addr{10, 0, 0, 0}, IP4::addr{255, 255, 255, 0}, 0, eth0, 1},
      {IP4::addr{10, 0, 20, 0}, IP4::addr{255, 255, 255, 0}, 0, eth1, 1},
      {IP4::addr{0}, IP4::addr{0}, IP4::addr{10, 0, 0, 1}, eth0, 1}};
  nacl_router_obj = std::make_unique<Router<IP4>>(routing_table);
  // Set ip forwarding on every iface mentioned in routing_table
  eth0.set_forward_delg(nacl_router_obj->forward_delg());
  eth1.set_forward_delg(nacl_router_obj->forward_delg());

  // Ct
  nacl_ct_obj = std::make_shared<Conntrack>();
  INFO("NaCl", "Enabling Conntrack on eth0");
  eth0.enable_conntrack(nacl_ct_obj);
  INFO("NaCl", "Enabling Conntrack on eth1");
  eth1.enable_conntrack(nacl_ct_obj);
}
