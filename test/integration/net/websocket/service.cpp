#include <service>
#include <net/interfaces>
#include <net/ws/connector.hpp>
#include <http>
#include <deque>
#include <kernel/crash_context.hpp>

struct alignas(SMP_ALIGN) HTTP_server
{
  http::Server*      server = nullptr;
  net::tcp::buffer_t buffer = nullptr;
  net::WS_server_connector* ws_serve = nullptr;
  // websocket clients
  std::deque<net::WebSocket_ptr> clients;
};
static SMP::Array<HTTP_server> httpd;

static net::WebSocket_ptr& new_client(net::WebSocket_ptr socket)
{
  auto& sys = PER_CPU(httpd);
  for (auto& client : sys.clients)
  if (client->is_alive() == false) {
    return client = std::move(socket);
  }

  sys.clients.push_back(std::move(socket));
  return sys.clients.back();
}

bool accept_client(net::Socket remote, std::string origin)
{
  /*
  printf("Verifying origin: \"%s\"\n"
         "Verifying remote: \"%s\"\n",
         origin.c_str(), remote.to_string().c_str());
  */
  (void) origin;
  return remote.address() == net::ip4::Addr(10,0,0,1);
}

void websocket_service(net::TCP& tcp, uint16_t port)
{
  PER_CPU(httpd).server = new http::Server(tcp);

  // buffer used for testing
  PER_CPU(httpd).buffer = net::tcp::construct_buffer(1024);

  // Set up server connector
  PER_CPU(httpd).ws_serve = new net::WS_server_connector(
    [&tcp] (net::WebSocket_ptr ws)
    {
      // sometimes we get failed WS connections
      if (ws == nullptr) return;
      SET_CRASH("WebSocket created: %s", ws->to_string().c_str());

      auto& socket = new_client(std::move(ws));
      // if we are still connected, attempt was verified and the handshake was accepted
      if (socket->is_alive())
      {
        socket->on_read =
        [] (auto message) {
          printf("WebSocket on_read: %.*s\n", (int) message->size(), message->data());
        };

        //socket->write("THIS IS A TEST CAN YOU HEAR THIS?");
        for (int i = 0; i < 1000; i++)
            socket->write(PER_CPU(httpd).buffer, net::op_code::BINARY);

        //socket->close();
        socket->on_close =
          [] (uint16_t reason) {
            if (reason == 1000)
              printf("SUCCESS\n");
            else
              assert(0 && "FAILURE");
          };
      }
    },
    accept_client);
  PER_CPU(httpd).server->on_request(*PER_CPU(httpd).ws_serve);
  PER_CPU(httpd).server->listen(port);
  /// server ///
}

void Service::start()
{
  auto& inet = net::Interfaces::get(0);
  inet.network_config(
      {  10, 0,  0, 54 },  // IP
      { 255,255,255, 0 },  // Netmask
      {  10, 0,  0,  1 },  // Gateway
      {  10, 0,  0,  1 }); // DNS

  // run websocket server locally
  websocket_service(inet.tcp(), 8000);
}

void ws_client_test(net::TCP& tcp)
{
  /// client ///
  static http::Basic_client client(tcp);
  net::WebSocket::connect(client, "ws://10.0.0.1:8001/",
  [] (net::WebSocket_ptr socket)
  {
    if (!socket) {
      printf("WS Connection failed!\n");
      return;
    }
    socket->on_error =
    [] (std::string reason) {
      printf("Socket error: %s\n", reason.c_str());
    };

    socket->write("HOLAS\r\n");
    PER_CPU(httpd).clients.push_back(std::move(socket));
  });
  /// client ///
}
