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
    : tcp_(tcp), conns_{}
  {
  }

  Request_ptr Client::create_request(Method method) const
  {
    auto req = std::make_unique<Request>();
    req->set_method(method);

    auto& header = req->header();
    header.set_field(header::User_Agent, "IncludeOS/0.9");
    set_connection_header(*req);

    return req;
  }

  void Client::send(Request_ptr req, Host host, Response_handler cb)
  {
    auto& conn = get_connection(host);

    auto&& header = req->header();
    // Set Host if not already set
    if(! header.has_field(header::Host))
      header.set_field(header::Host, host.to_string());

    conn.send(std::move(req), std::move(cb));
  }

  void Client::request(Method method, URI url, Header_set hfields, Response_handler cb)
  {
    resolve(url,
    [ this, method, url{std::move(url)}, hfields{std::move(hfields)}, cb{std::move(cb)} ] (auto ip)
    {
      // Host resolved
      if(ip != 0)
      {
        // setup request with method and headers
        auto req = create_request(method);
        *req << hfields;

        // Set Host and URI path
        populate_from_url(*req, url);

        // Default to port 80 if non given
        const uint16_t port = (url.port() != 0xFFFF) ? url.port() : 80;

        send(std::move(req), {ip, port}, std::move(cb));
      }
      else
      {
        cb({Error::RESOLVE_HOST}, nullptr);
      }
    });
  }

  void Client::request(Method method, Host host, std::string path, Header_set hfields, Response_handler cb)
  {
    // setup request with method and headers
    auto req = create_request(method);
    *req << hfields;

    //set uri
    req->set_uri(URI{std::move(path)});

    send(std::move(req), std::move(host), std::move(cb));
  }

  void Client::request(Method method, URI url, Header_set hfields, std::string data, Response_handler cb)
  {
    resolve(url,
    [ this, method, url, hfields{std::move(hfields)}, data{std::move(data)}, cb{std::move(cb)} ] (auto ip)
    {
      // Host resolved
      if(ip != 0)
      {
        // setup request with method and headers
        auto req = create_request(method);
        *req << hfields;

        // Set Host & path from url
        populate_from_url(*req, url);

        // Add data and content length
        add_data(*req, data);

        // Default to port 80 if non given
        const uint16_t port = (url.port() != 0xFFFF) ? url.port() : 80;

        send(std::move(req), {ip, port}, std::move(cb));
      }
      else
      {
        cb({Error::RESOLVE_HOST}, nullptr);
      }
    });
  }

  void Client::request(Method method, Host host, std::string path, Header_set hfields, const std::string& data, Response_handler cb)
  {
    // setup request with method and headers
    auto req = create_request(method);
    *req << hfields;

    // set uri
    req->set_uri(URI{std::move(path)});

    // Add data and content length
    add_data(*req, data);

    send(std::move(req), std::move(host), std::move(cb));
  }

  void Client::add_data(Request& req, const std::string& data)
  {
    auto& header = req.header();
    if(!header.has_field(header::Content_Type))
      header.set_field(header::Content_Type, "text/plain");

    // Set Content-Length to be equal data length
    req.header().set_field(header::Content_Length, std::to_string(data.size()));

    // Add data
    req.add_body(data);
  }

  void Client::populate_from_url(Request& req, const URI& url)
  {
    // Set uri path
    req.set_uri( URI{url.path().to_string()} );
    // Set Host: host(:port)
    req.header().set_field(header::Host,
      (url.port() != 0xFFFF) ?
      url.host().to_string() + ":" + url.port_str().to_string()
      : url.host().to_string()); // to_string madness
  }

  void Client::resolve(const URI& url, ResolveCallback cb)
  {
    static auto&& stack = tcp_.stack();
    stack.resolve(url.host().to_string(),
      [ cb{std::move(cb)} ](auto ip)
    {
      cb(ip);
    });
  }

  Connection& Client::get_connection(const Host host)
  {
    // return/create a set for the given host
    auto& cset = conns_[host];

    // iterate all the connection and return the first free one
    for(auto& conn : cset)
    {
      if(!conn->occupied())
        return *conn;
    }

    // no non-occupied connections, emplace a new one
    cset.push_back(std::make_unique<Connection>(tcp_.connect(host), Connection::Close_handler{this, &Client::close}));
    return *cset.back();
  }

  void Client::close(Connection& c)
  {
    debug("Closing %u:%s %p\n", c.local_port(), c.peer().to_string().c_str(), &c);
    auto& cset = conns_.at(c.peer());

    cset.erase(std::remove_if(cset.begin(), cset.end(),
    [port = c.local_port()] (const std::unique_ptr<Connection>& conn)->bool
    {
      return conn->local_port() == port;
    }));
  }

}
