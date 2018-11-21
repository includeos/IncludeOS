#include "balancer.hpp"
#include <net/tcp/stream.hpp>

namespace microLB
{
  // default method for opening a TCP port for clients
  void Balancer::open_for_tcp(
        netstack_t&    interface,
        const uint16_t client_port)
  {
    interface.tcp().listen(client_port,
    [this] (auto conn) {
      assert(conn != nullptr && "TCP sanity check");
      this->incoming(std::make_unique<net::tcp::Stream> (conn));
    });
  }
  // default method for TCP nodes
  node_connect_function_t Balancer::connect_with_tcp(
          netstack_t& interface,
          net::Socket socket)
  {
return [&interface, socket] (timeout_t timeout, node_connect_result_t callback)
    {
      auto conn = interface.tcp().connect(socket);
      assert(conn != nullptr && "TCP sanity check");
      // cancel connect after timeout
      int timer = Timers::oneshot(timeout,
          Timers::handler_t::make_packed(
          [conn, callback] (int) {
            conn->abort();
            callback(nullptr);
          }));
      conn->on_connect(
        net::tcp::Connection::ConnectCallback::make_packed(
        [timer, callback] (auto conn) {
          // stop timeout after successful connect
          Timers::stop(timer);
          if (conn != nullptr) {
            // the connect() succeeded
            assert(conn->is_connected() && "TCP sanity check");
            callback(std::make_unique<net::tcp::Stream>(conn));
          }
          else {
            // the connect() failed
            callback(nullptr);
          }
        }));
    };
  }
}
