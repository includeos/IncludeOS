#include <net/interfaces>
#include <net/ws/connector.hpp>
#include "fuzzy_helpers.hpp"
#include "fuzzy_http.hpp"
#include "fuzzy_stream.hpp"

extern http::Response_ptr handle_request(const http::Request&);
static struct upper_layer
{
  fuzzy::HTTP_server*   server = nullptr;
  net::WS_server_connector* ws_serve = nullptr;
} httpd;

static bool accept_client(net::Socket remote, std::string origin)
{
  (void) origin; (void) remote;
  //return remote.address() == net::ip4::Addr(10,0,0,1);
  return true;
}

void fuzzy_http(const uint8_t* data, size_t size)
{
  // Upper layer fuzzing using fuzzy::Stream
  auto& inet = net::Interfaces::get(0);
  static bool init_http = false;
  if (UNLIKELY(init_http == false)) {
    init_http = true;
    // server setup
    httpd.server = new fuzzy::HTTP_server(inet.tcp());
    httpd.server->on_request(
      [] (http::Request_ptr request,
          http::Response_writer_ptr response_writer)
      {
        response_writer->set_response(handle_request(*request));
        response_writer->write();
      });
    httpd.server->listen(80);
  }
  fuzzy::FuzzyIterator fuzzer{data, size};
  // create HTTP stream
  const net::Socket local  {inet.ip_addr(), 80};
  const net::Socket remote {{10,0,0,1}, 1234};
  auto http_stream = std::make_unique<fuzzy::Stream> (local, remote,
    [] (net::Stream::buffer_t buffer) {
      //printf("Received %zu bytes on fuzzy stream\n%.*s\n",
      //      buffer->size(), (int) buffer->size(), buffer->data());
      (void) buffer;
    });
  auto* test_stream = http_stream.get();
  httpd.server->add(std::move(http_stream));
  // random websocket stuff
  auto buffer = net::Stream::construct_buffer();
  fuzzer.insert(buffer, fuzzer.size);
  test_stream->give_payload(std::move(buffer));

  // close stream from our end
  test_stream->transport_level_close();
}

void fuzzy_websocket(const uint8_t* data, size_t size)
{
  // Upper layer fuzzing using fuzzy::Stream
  auto& inet = net::Interfaces::get(0);
  static bool init_http = false;
  if (UNLIKELY(init_http == false)) {
    init_http = true;
    // server setup
    httpd.server = new fuzzy::HTTP_server(inet.tcp());
    // websocket setup
    httpd.ws_serve = new net::WS_server_connector(
      [] (net::WebSocket_ptr ws)
      {
        // sometimes we get failed WS connections
        if (ws == nullptr) return;

        auto wptr = ws.release();
        // if we are still connected, attempt was verified and the handshake was accepted
        assert (wptr->is_alive());
        wptr->on_read =
        [] (auto message) {
          printf("WebSocket on_read: %.*s\n", (int) message->size(), message->data());
        };
        wptr->on_close =
        [wptr] (uint16_t) {
          delete wptr;
        };

        //wptr->write("THIS IS A TEST CAN YOU HEAR THIS?");
        wptr->close();
      },
      accept_client);
    httpd.server->on_request(*httpd.ws_serve);
  }

  fuzzy::FuzzyIterator fuzzer{data, size};
  // create HTTP stream
  const net::Socket local  {inet.ip_addr(), 80};
  const net::Socket remote {{10,0,0,1}, 1234};
  auto http_stream = std::make_unique<fuzzy::Stream> (local, remote,
    [] (net::Stream::buffer_t buffer) {
      //printf("Received %zu bytes on fuzzy stream\n%.*s\n",
      //      buffer->size(), (int) buffer->size(), buffer->data());
      (void) buffer;
    });
  auto* test_stream = http_stream.get();
  httpd.server->add(std::move(http_stream));
  auto buffer = net::Stream::construct_buffer();
  // websocket HTTP upgrade
  const std::string webs = "GET / HTTP/1.1\r\n"
        "Host: www.fake.com\r\n"
        "Upgrade: WebSocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "Origin: http://www.fake.com\r\n"
        "\r\n";
  buffer->insert(buffer->end(), webs.c_str(), webs.c_str() + webs.size());
  //printf("Request: %.*s\n", (int) buffer->size(), buffer->data());
  test_stream->give_payload(std::move(buffer));

  // random websocket stuff
  buffer = net::Stream::construct_buffer();
  fuzzer.insert(buffer, fuzzer.size);
  test_stream->give_payload(std::move(buffer));

  // close stream from our end
  test_stream->transport_level_close();
}
