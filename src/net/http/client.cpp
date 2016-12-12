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
    : tcp_(tcp)
  {
  }

  void Client::send(Request_ptr req, Host host, ResponseCallback cb)
  {
    auto& conn = get_connection(host);

    printf("Sending Request:\n%s\n", req->to_string().c_str());
    conn.send(std::move(req), std::move(cb));
  }

  void Client::get(URI url, Header_set hfields, ResponseCallback cb)
  {
    resolve(url,
    [ this, url{std::move(url)}, cb{std::move(cb)}, hfields{std::move(hfields)} ] (auto ip)
    {
      // Host resolved
      if(ip != 0)
      {
        const uint16_t port = (url.port() != 0xFFFF) ? url.port() : 80;

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
    req.set_uri( URI{url.path().to_string()} );
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

  Connection& Client::get_connection(const Host host)
  {
    // return/create a set for the given host
    auto& cset = conns_[host];

    // iterate all the connection and return the first free one
    for(auto& conn : cset)
    {
      if(!conn.occupied())
        return conn;
    }

    // no non-occupied connections, emplace a new one
    cset.emplace_back(tcp_.connect(host), Connection::Close_handler{this, &Client::close});
    return cset.back();
  }

  void Client::close(Connection& c)
  {
    auto& cset = conns_.at(c.peer());

    cset.erase(std::remove_if(cset.begin(), cset.end(),
    [port = c.local_port()] (const Connection& conn)->bool
    {
      return conn.local_port() == port;
    }));
  }

}
