#include "ircd.hpp"

IrcServer::IrcServer(
    Network& inet, uint16_t port, const std::string& name)
    : network(inet), server_name(name)
{
  auto& tcp = network.tcp();
  auto& server_port = tcp.bind(port);
  server_port.onConnect(
  [this] (auto csock)
  {
    printf("*** Received connection from %s\n",
    csock->remote().to_string().c_str());

    /// create client ///
    size_t clindex = free_client();
    auto& client = clients[clindex];
    client.set_connection(csock);

    // set up callbacks
    csock->read(1024,
    [this, &client] (net::TCP::buffer_t buffer, size_t bytes)
    {
      client.read(buffer.get(), bytes);
    });

    csock->onDisconnect(
    [this, &client] (auto, std::string)
    {
      // mark as disabled
      client.disable();
      // remove client from various lists and
      for (size_t idx : client.channels()) {
        get_channel(idx).remove(client.get_id());
      }
      /// inform others about disconnect
      //client.bcast(TK_QUIT, "Disconnected");
    });
  });
}

size_t IrcServer::free_client() {
  // use prev dead client
  if (!free_clients.empty()) {
    size_t idx = free_clients.back();
    free_clients.pop_back();
    return idx;
  }
  // create new client
  clients.emplace_back(clients.size(), *this);
  return clients.size()-1;
}
size_t IrcServer::free_channel() {
  // use prev dead channel
  if (!free_channels.empty()) {
    size_t idx = free_channels.back();
    free_channels.pop_back();
    return idx;
  }
  // create new channel
  channels.emplace_back(channels.size(), *this);
  return channels.size()-1;
}
