#pragma once
#include <stdexcept>

static inline
void server(net::Inet<net::IP4>& inet, 
            const uint16_t port, 
            delegate<void(liu::buffer_len&)> callback)
{
  auto& server = inet.tcp().bind(port);
  server.on_connect(
  net::tcp::Connection::ConnectCallback::make_packed(
  [callback, port] (auto conn)
  {
    static const int UPDATE_MAX = 1024*1024 * 3; // 3mb files max
    auto* buffer = new liu::buffer_len {new char[UPDATE_MAX], 0};
    printf("Receiving blob on port %u\n", port);

    // retrieve binary
    conn->on_read(9000,
    [conn, buffer] (net::tcp::buffer_t buf, size_t n)
    {
      if (buffer->length + n > UPDATE_MAX) return;
      memcpy((char*) &buffer->buffer[buffer->length], buf.get(), n);
      buffer->length += n;

    }).on_close(
    net::tcp::Connection::CloseCallback::make_packed(
    [buffer, callback] {
      float frac = buffer->length / (float) UPDATE_MAX * 100.f;
      printf("* Blob size: %u b  (%.2f%%) stored at %p\n", 
             buffer->length, frac, buffer->buffer);
      callback(*buffer);
      delete[] buffer->buffer;
      delete   buffer;
    }));
  }));
}
