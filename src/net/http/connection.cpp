// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016 Oslo and Akershus University College of Applied Sciences
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

#include <net/http/connection.hpp>
#include <gsl/gsl_assert>

namespace http {

  Connection::Connection(TCP_conn_ptr tcpconn, Close_handler on_close)
    : tcpconn_{std::move(tcpconn)},
      req_{nullptr},
      res_{nullptr},
      on_close_{std::move(on_close)},
      on_response_{},
      keep_alive_{true}
  {
    // setup close event
    tcpconn_->on_close({this, &Connection::close});

    //tcpconn_->on_connect([](auto self) {
    //  printf("<http::Connection> Connected: %s\n", self->to_string().c_str());
    //});
  }
  template <typename TCP>
  Connection::Connection(TCP& tcp, Peer addr, Close_handler on_close)
    : Connection(tcp.connect(addr), std::move(on_close))
  {
  }

  void Connection::send(Request_ptr req, Response_handler on_res, const size_t bufsize)
  {
    req_ = std::move(req);
    res_ = std::make_unique<Response>();
    on_response_ = std::move(on_res);

    send_request(bufsize);
  }

  void Connection::send_request(const size_t bufsize)
  {
    keep_alive_ = (req_->header().value(header::Connection) != "close");

    tcpconn_->on_read(bufsize, {this, &Connection::recv_response});

    tcpconn_->write(req_->to_string());
  }

  void Connection::recv_response(buffer_t buf, size_t len)
  {
    Expects(res_ != nullptr);
    *res_ << std::string{(char*)buf.get(), len};
    res_->parse();

    const auto& header = res_->header();
    // TODO: Temporary, not good enough
    // if(res_->is_complete())
    if(!header.is_empty() && std::stoi(header.value(header::Content_Length).to_string()) == res_->body().size())
      end_response();
  }

  void Connection::end_response()
  {
    // move response to a copy in case of callback result in new request
    auto callback{std::move(on_response_)};

    callback({Error::NONE}, std::move(res_));

    // user callback may override this
    if(keep_alive_)
      tcpconn_->close();
  }

}
