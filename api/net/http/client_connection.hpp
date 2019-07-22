
#pragma once
#ifndef HTTP_CLIENT_CONNECTION_HPP
#define HTTP_CLIENT_CONNECTION_HPP

// http
#include "common.hpp"
#include "connection.hpp"
#include "error.hpp"

#include <util/timer.hpp>

namespace http {

  class Basic_client;

  class Client_connection : public Connection {
  public:
    using Response_handler  = delegate<void(Error, Response_ptr, Connection&)>;
    using timeout_duration  = std::chrono::milliseconds;

  public:
    explicit Client_connection(Basic_client&, Stream_ptr);

    bool available() const
    { return on_response_ == nullptr && keep_alive_; }

    bool occupied() const
    { return !available(); }

    void send(Request_ptr, Response_handler, int redirects, timeout_duration = timeout_duration::zero());

  private:
    Basic_client&     client_;
    Request_ptr       req_;
    Response_ptr      res_;
    Response_handler  on_response_;
    Timer             timer_;
    timeout_duration  timeout_dur_;
    int               redirect_;

    void send_request();

    void recv_response(buffer_t buf);

    void end_response(Error err = Error::NONE);

    void timeout_request()
    { end_response(Error::TIMEOUT); }

    bool can_redirect(const Response_ptr&) const;

    void redirect(uri::URI url);

    void close() override;

  }; // < class Client_connection

} // < namespace http

#endif // < HTTP_CLIENT_CONNECTION_HPP
