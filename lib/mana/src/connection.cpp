// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "../include/mana/connection.hpp"
#include "../include/mana/server.hpp"

#include <kernel/syscalls.hpp>

using namespace mana;

size_t Connection::PAYLOAD_LIMIT = 1024*16;
Connection::OnConnection Connection::on_connection_ = []{};

Connection::Connection(Server& serv, Connection_ptr conn, size_t idx)
  : server_(serv), conn_(conn), idx_(idx)
{
  conn_->on_read(BUFSIZE, {this, &Connection::on_data});
  conn_->on_disconnect({this, &Connection::on_disconnect});
  conn_->on_close({this, &Connection::close});
  conn_->on_error({this, &Connection::on_error});
  conn_->on_packet_dropped({this, &Connection::on_packet_dropped});
  //conn_->on_rtx_timeout([](auto, auto) { printf("<TCP> RtxTimeout\n"); });
  on_connection_();
  idle_since_ = RTC::now();
}

void Connection::on_data(buffer_t buf, size_t n) {
  //printf("Connection::on_data: %*s", n, buf.get());
  SET_CRASH_CONTEXT("Connection::on_data: data from %s\n\n%*s",
    conn_->to_string().c_str(), n, buf.get());
  #ifdef VERBOSE_WEBSERVER
  printf("<%s> @on_data, size=%u\n", to_string().c_str(), n);
  #endif
  int client_error = 0;
  // if it's a new request
  if(!request_) {
    try
    {
      request_ = std::make_shared<Request>(buf, n);
      update_idle();

      request_->validate();

      if(request_->content_length() > PAYLOAD_LIMIT)
        client_error = http::Payload_Too_Large;
    }
    catch(const Request_error& e)
    {
      client_error = e.code();
      printf("<%s> Request error - %s\n",
        to_string().c_str(), e.what());
    }
    catch(const std::exception& e)
    {
      client_error = http::Bad_Request;
      printf("<%s> HTTP Error - %s\n",
        to_string().c_str(), e.what());
    }
  }
  // else we assume it's payload
  else
  {
    printf("<%s> Received payload: [%u / %u]\n",
        to_string().c_str(), request_->payload_length()+n, request_->content_length());
    // is valid payload length
    if(request_->payload_length() + n <= PAYLOAD_LIMIT)
    {
      update_idle();
      request_->add_chunk(std::string{(const char*)buf.get(), n});
    }
    else
    {
      client_error = http::Payload_Too_Large;
    }
  }

  // if there was any error on the request
  // note: this is probably a bit too aggressive - client should have another try on some cases
  if(client_error)
  {
    Response res{conn_};
    res.send_code(static_cast<http::status_t>(client_error), true);
    close_tcp();
    response_ = nullptr;
    request_ = nullptr;
    return;
  }

  // if there is more data to be received, buffer
  if(request_->should_buffer()) {
    printf("<%s> [ConLen (%u) > Payload (%u)] => Buffering\n",
        to_string().c_str(), request_->content_length(), request_->payload_length());
    return; // buffer
  }

  response_ = std::make_shared<Response>(conn_);

  request_->complete();

  #ifdef VERBOSE_WEBSERVER
  printf("<%s> Complete Request: [%s] Payload (%u/%u B)\n",
    to_string().c_str(),
    request_->route_string().c_str(),
    request_->payload_length(),
    request_->content_length()
    );
  #endif

  server_.process(request_, response_);
  request_ = nullptr;
}

void Connection::on_disconnect(Connection_ptr, Disconnect reason) {
  update_idle();
  (void)reason;
  #ifdef VERBOSE_WEBSERVER
  printf("<%s> Disconnect: %s\n",
    to_string().c_str(), reason.to_string().c_str());
  #endif
  close_tcp();
}

void Connection::on_error(TCPException err) {
  printf("<%s> TCP Error: %s\n",
    to_string().c_str(), err.what());
}

void Connection::on_packet_dropped(const Packet_ptr::element_type&, const std::string& reason) {
  printf("<%s> Packet dropped: %s\n",
    to_string().c_str(), reason.c_str());
}

void Connection::close() {
  request_ = nullptr;
  response_ = nullptr;
  server_.close(idx_);
}

void Connection::timeout() {
  conn_->is_closing() ? conn_->abort() : conn_->close();
}

Connection::~Connection() {
  #ifdef VERBOSE_WEBSERVER
  printf("<%s> Deleted\n", to_string().c_str());
  #endif
}
