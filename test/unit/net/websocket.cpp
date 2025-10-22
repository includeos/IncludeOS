#include <common.cxx>
#include <net/inet>
#include <net/interfaces>
#include <hw/async_device.hpp>

#include <http>
#include <net/ws/connector.hpp>
extern http::Response_ptr handle_request(const http::Request&);
static struct upper_layer
{
  http::Server* server = nullptr;
  net::WS_server_connector* ws_serve = nullptr;
} httpd;

static bool accept_client(net::Socket remote, std::string origin)
{
  (void) origin; (void) remote;
  //return remote.address() == net::ip4::Addr(10,0,0,1);
  return true;
}

static std::unique_ptr<hw::Async_device<UserNet>> dev1 = nullptr;
static std::unique_ptr<hw::Async_device<UserNet>> dev2 = nullptr;

static void setup_inet()
{
  dev1 = std::make_unique<hw::Async_device<UserNet>>(UserNet::create(1500));
  dev2 = std::make_unique<hw::Async_device<UserNet>>(UserNet::create(1500));
  dev1->connect(*dev2);
  dev2->connect(*dev1);

  auto& inet_server = net::Interfaces::get(0);
  inet_server.network_config({10,0,0,42}, {255,255,255,0}, {10,0,0,43});
  auto& inet_client = net::Interfaces::get(1);
  inet_client.network_config({10,0,0,43}, {255,255,255,0}, {10,0,0,42});
}

static void setup_websocket_server()
{
  auto& inet = net::Interfaces::get(0);
  // server setup
  httpd.server = new http::Server(inet.tcp());
  /*
  httpd.server->on_request(
    [] (http::Request_ptr request,
        http::Response_writer_ptr response_writer)
    {
      response_writer->set_response(handle_request(*request));
      response_writer->write();
    });
  */
  // websocket setup
  httpd.ws_serve = new net::WS_server_connector(
    [] (net::WebSocket_ptr ws)
    {
      printf("WebSocket connector %p\n", ws.get());
      // sometimes we get failed WS connections
      if (ws == nullptr) return;

      auto wptr = ws.release();
      // if we are still connected, attempt was verified and the handshake was accepted
      assert (wptr->is_alive());
      wptr->on_read =
      [wptr] (auto message) {
        printf("WebSocket on_read: %s\n", message->to_string().c_str());
        wptr->write(message->extract_shared_vector());
      };
      wptr->on_close =
      [wptr] (uint16_t) {
        printf("WebSocket server close\n");
        delete wptr;
      };

      //wptr->write("THIS IS A TEST CAN YOU HEAR THIS?");
      //wptr->close();
    },
    accept_client);
  httpd.server->on_request(*httpd.ws_serve);
  httpd.server->listen(55);
}

typedef delegate<void(net::WebSocket_ptr, bool&)> do_thing_t;
static void websocket_do_thing(do_thing_t callback)
{
  // create HTTP stream
  auto& inet = net::Interfaces::get(1);
  auto http_client = std::make_unique<http::Basic_client>(inet.tcp());

  static bool done = false;
  net::WebSocket::connect(*http_client, "ws://10.0.0.42:55",
      net::WebSocket::Connect_handler::make_packed(
      [callback] (net::WebSocket_ptr ws) {
        printf("Connected ws=%p\n", ws.get());
        callback(std::move(ws), done);
      }));
  while (!done)
  {
    //printf("BEF Done = %d\n", done);
    Events::get().process_events();
    //printf("AFT Done = %d\n", done);
  }
}

CASE("Setup websocket server")
{
  Timers::init(
    [] (Timers::duration_t) {},
    [] () {}
  );
  setup_inet();
  setup_websocket_server();
}

CASE("Open and close websocket")
{
  // FIXME
  return;
  websocket_do_thing(
    [] (net::WebSocket_ptr webs, bool& done)
    {
      auto* ws = webs.release();
      ws->on_close =
      [&] (uint16_t reason) {
        printf("Reason: %s (%d)\n", net::WebSocket::status_code(reason), reason);
        assert(std::string(net::WebSocket::status_code(reason)) == "Closed");
        done = true;
        printf("Client closed\n");
        delete ws;
      };
      // FIXME
      // close() doesn't work because of infinite loop?
      printf("Closing client\n");
      ws->close();
    });
}

CASE("Send some short websocket data")
{
  static const std::string data_string = "There is data in this string";
  websocket_do_thing(
    [] (net::WebSocket_ptr webs, bool& done)
    {
      auto* ws = webs.release();
      ws->on_read =
        [&] (auto msg) {
          printf("WebSocket: Read back!\n");
          assert(msg->to_string() == data_string);
          done = true;
        };
      ws->write(data_string);
    });
}
