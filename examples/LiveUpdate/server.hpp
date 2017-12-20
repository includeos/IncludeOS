/**
 * Master thesis
 * by Alf-Andre Walla 2016-2017
 *
**/
#pragma once
#include <stdexcept>

static inline
void server(net::Inet<net::IP4>& inet,
            const uint16_t port,
            delegate<void(liu::buffer_t&)> callback)
{
  auto& server = inet.tcp().listen(port);
  server.on_connect(
  net::tcp::Connection::ConnectCallback::make_packed(
  [callback, port] (auto conn)
  {
    auto* buffer = new liu::buffer_t;
    buffer->reserve(3*1024*1024);
    printf("Receiving blob on port %u\n", port);

    // retrieve binary
    conn->on_read(9000,
    [conn, buffer] (auto buf)
    {
      buffer->insert(buffer->end(), buf->begin(), buf->end());
    })
    .on_disconnect(
    net::tcp::Connection::DisconnectCallback::make_packed(
    [buffer, callback] (auto conn, auto) {
      printf("* Blob size: %u b  stored at %p\n",
            (uint32_t) buffer->size(), buffer->data());
      callback(*buffer);
      delete buffer;
      conn->close();
    }));
  }));
}
