
#pragma once
#ifndef HTTP_SERVER_CONNECTION_HPP
#define HTTP_SERVER_CONNECTION_HPP

// http
#include "connection.hpp"

#include <rtc>

namespace http {

  class Server;

  class Server_connection : public Connection {
  public:
    static constexpr size_t DEFAULT_BUFSIZE = 1460;

  public:
    explicit Server_connection(Server&, Stream_ptr, size_t idx, const size_t bufsize = DEFAULT_BUFSIZE);

    void send(Response_ptr res);

    size_t idx() const noexcept
    { return idx_; }

    auto idle_since() const noexcept
    { return idle_since_; }

  private:
    Server&           server_;
    Request_ptr       req_;
    size_t            idx_;
    RTC::timestamp_t  idle_since_;

    void recv_request(buffer_t);

    void end_request(status_t code = http::OK);

    void close() override;

    void update_idle()
    { idle_since_ = RTC::now(); }

  }; // < class Server_connection



} // < namespace http

#endif // < HTTP_SERVER_CONNECTION_HPP
