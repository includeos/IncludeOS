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

#include <net/http/client.hpp>

namespace http {

  Client::Client(TCP& tcp)
    : tcp_(tcp), conn_(nullptr)
  {
  }

  void Client::send(Request_ptr req, Host host, ResponseCallback cb)
  {
    printf("Sending Request:\n%s\n", req->to_string().c_str());
    conn_ = tcp_.connect(host);

    conn_->on_read(bufsiz,
    [ conn = conn_, cb{std::move(cb)} ] (auto buf, auto len)
    {
      printf("Client@onRead:\n%.*s\n", len, (char*) buf.get());
      if(len == 0) {
        cb({Error::NO_REPLY}, nullptr);
        return;
      }
      try {
        auto res = make_response({(char*)buf.get(), len});
        cb({Error::NONE}, std::move(res));
      }
      catch(...)
      {
        cb({Error::NO_REPLY}, nullptr);
      }
    });

    conn_->write(req->to_string());
  }

  void Client::get(URI url, Header_set hfields, ResponseCallback cb)
  {
    resolve(url,
    [ this, url, cb{std::move(cb)}, hfields{std::move(hfields)} ] (auto ip)
    {
      // Host resolved
      if(ip != 0)
      {
        const uint16_t port = (url.port() != 0) ? url.port() : 80;

        auto req = create_request();

        populate_from_url(*req, url);

        *req << hfields;

        send(std::move(req), {ip, port}, std::move(cb));
      }
      else
      {
        cb({Error::RESOLVE_HOST}, nullptr);
      }
    });
  }

  void Client::post(URI url, Header_set hfields, std::string data, ResponseCallback cb)
  {
    resolve(url,
    [ this, url, cb{std::move(cb)}, data{std::move(data)}, hfields{std::move(hfields)} ] (auto ip)
    {
      // Host resolved
      if(ip != 0)
      {
        const uint16_t port = (url.port() != 0) ? url.port() : 80;

        auto req = create_request();

        req->set_method(POST);

        populate_from_url(*req, url);

        *req << hfields;

        req->add_body(std::move(data));

        send(std::move(req), {ip, port}, std::move(cb));
      }
      else
      {
        cb({Error::RESOLVE_HOST}, nullptr);
      }
    });
  }

  void Client::populate_from_url(Request& req, const URI& url)
  {
    // Set uri path
    req.set_uri( URI{url.path()} );
    // Set Host: url.host
    req.header().set_field(header::Host, url.host().to_string());
  }

  void Client::resolve(const URI& url, ResolveCallback cb)
  {
    static auto&& stack = tcp_.stack();
    stack.resolve(url.host().to_string(),
      [cb](auto ip)
    {
      cb(ip);
    });
  }

  Request_ptr Client::create_request() const
  {
    auto req = std::make_unique<Request>();

    auto& header = req->header();
    header.set_field(header::User_Agent, "IncludeOS/0.9");
    set_connection_header(*req);

    return req;
  }

}
