#include "ircd/ircd.hpp"
#include <config>
#include <rapidjson/document.h>

std::unique_ptr<IrcServer> IrcServer::from_config()
{
  rapidjson::Document doc;
  doc.Parse(Config::get().data());

  if (doc.IsObject() == false || doc.HasMember("ircd") == false)
      throw std::runtime_error("Missing or invalid configuration");

  const auto& obj = doc["ircd"];

  // client interface
  const int CLIENT_NET = obj["client_iface"].GetInt();
  auto& clinet = net::Super_stack::get<net::IP4>(CLIENT_NET);
  const int CLIENT_PORT = obj["client_port"].GetUint();
  assert(CLIENT_PORT > 0 && CLIENT_PORT < 65536);
  // server interface
  const int SERVER_NET = obj["server_iface"].GetInt();
  auto& srvinet = net::Super_stack::get<net::IP4>(SERVER_NET);
  const int SERVER_PORT = obj["server_port"].GetUint();
  assert(SERVER_PORT > 0 && SERVER_PORT < 65536);

  // unique server ID
  const int server_id = obj["server_id"].GetInt();
  // server name
  const std::string server_name = obj["server_name"].GetString();
  // server network name
  const std::string server_netw = obj["server_netw"].GetString();

  return std::unique_ptr<IrcServer>{new IrcServer(
                clinet, CLIENT_PORT, srvinet, SERVER_PORT,
                server_id, server_name, server_netw)};
}
