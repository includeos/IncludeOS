#include "balancer.hpp"
#include <config>
#include <rapidjson/document.h>

template <typename T>
static net::Socket parse_socket(T& obj)
{
  assert(obj.HasMember("port"));
  // uses regex
  return {{obj["address"].GetString()}, obj["port"].GetUint()};
}

std::vector<net::Socket> Balancer::parse_node_confg()
{
  rapidjson::Document doc;
  doc.Parse(Config::get().data());
  std::vector<net::Socket> result;

  if (doc.IsObject() && doc.HasMember("piranha"))
  {
    const auto& obj = doc["piranha"];
    if (obj.HasMember("nodes"))
    {
      auto& nodes = obj["nodes"];
      assert(nodes.IsArray());
      for (auto& node : nodes.GetArray())
      {
        // nodes contain an array of [addr, port]
        assert(node.IsArray());
        const auto addr = node.GetArray();
        assert(addr.Size() == 2);
        // port must be valid
        unsigned port = addr[1].GetUint();
        assert(port >= 0 && port < 65536);
        // try to construct socket from string
        net::Socket socket{
          {addr[0].GetString()}, (uint16_t) port
        };
        result.push_back(socket);
      }
    } // nodes
  } // piranha
  return result;
} // parse_config()
