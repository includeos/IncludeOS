#include "balancer.hpp"
#include <config>
#include <rapidjson/document.h>
#include <net/interfaces.hpp>

namespace microLB
{
  Balancer* Balancer::from_config()
  {
    rapidjson::Document doc;
    doc.Parse(Config::get().data());

    if (doc.IsObject() == false || doc.HasMember("load_balancer") == false)
        throw std::runtime_error("Missing or invalid configuration");

    const auto& obj = doc["load_balancer"];

    auto& clients = obj["clients"];
    // client network interface
    const int CLIENT_NET = clients["iface"].GetInt();
    auto& netinc = net::Interfaces::get(CLIENT_NET);
    // client port
    const int CLIENT_PORT = clients["port"].GetUint();
    assert(CLIENT_PORT > 0 && CLIENT_PORT < 65536);
    // client wait queue limit
    const int CLIENT_WAITQ = clients["waitq_limit"].GetUint();
    (void) CLIENT_WAITQ;
    // client session limit
    const int CLIENT_SLIMIT = clients["session_limit"].GetUint();
    (void) CLIENT_SLIMIT;

    auto& nodes = obj["nodes"];
    // node interface
    const int NODE_NET = nodes["iface"].GetInt();
    auto& netout = net::Interfaces::get(NODE_NET);
    // node active-checks
    bool use_active_check = true;
    if (nodes.HasMember("active_check")) {
      use_active_check = nodes["active_check"].GetBool();
    }

    // create closed load balancer
    auto* balancer = new Balancer(use_active_check);

    if (clients.HasMember("certificate"))
    {
      assert(clients.HasMember("key") && "TLS-enabled microLB must also have key");
      // open for load balancing over TLS
      balancer->open_for_ossl(netinc, CLIENT_PORT,
            clients["certificate"].GetString(),
            clients["key"].GetString());
    }
    else {
      // open for TCP connections
      balancer->open_for_tcp(netinc, CLIENT_PORT);
    }
    // by default its this interface for nodes
    balancer->de_helper.nodes = &netout;

    auto& nodelist = nodes["list"];
    assert(nodelist.IsArray());
    for (auto& node : nodelist.GetArray())
    {
      // nodes contain an array of [addr, port]
      assert(node.IsArray());
      const auto addr = node.GetArray();
      assert(addr.Size() == 2);
      // port must be valid
      unsigned port = addr[1].GetUint();
      assert(port > 0 && port < 65536 && "Port is a number between 1 and 65535");
      // try to construct socket from string
      net::Socket socket{
        net::ip4::Addr{addr[0].GetString()}, (uint16_t) port
      };
      balancer->nodes.add_node(socket, Balancer::connect_with_tcp(netout, socket));
    }

#if defined(LIVEUPDATE)
    balancer->init_liveupdate();
#endif
    return balancer;
  }
}
