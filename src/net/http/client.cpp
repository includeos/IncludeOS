// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
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

  const Client::timeout_duration Client::DEFAULT_TIMEOUT{std::chrono::seconds(5)};

  Client::Client(TCP& tcp, Request_handler on_send)
    : tcp_(tcp),
      on_send_{std::move(on_send)},
      conns_{}
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

  void Client::send(Request_ptr req, Host host, Response_handler cb, Options options)
  {
    Expects(cb != nullptr);
    using namespace std;
    auto& conn = get_connection(host);

    auto&& header = req->header();
    // Set Host if not already set
    if(! header.has_field(header::Host))
      header.set_field(header::Host, host.to_string());

    debug("<http::Client> Sending Request:\n%s\n", req->to_string().c_str());

    if(on_send_)
      on_send_(*req, options, host);

    conn.send(move(req), move(cb), options.bufsize, options.timeout);
  }

  void Client::request(Method method, URI url, Header_set hfields, Response_handler cb, Options options)
  {
    Expects(cb != nullptr);
    using namespace std;

    if (url.host_is_ip4())
    {
      std::string host = url.host().to_string();
      auto ip = net::ip4::Addr(host);
      // setup request with method and headers
      auto req = create_request(method);
      *req << hfields;

      // Set Host and URI path
      populate_from_url(*req, url);

      // Default to port 80 if non given
      const uint16_t port = (url.port() != 0xFFFF) ? url.port() : 80;

      send(move(req), {ip, port}, move(cb), move(options));
    }
    else
    {
      tcp_.stack().resolve(url.host().to_string(),
      ResolveCallback::make_packed(
      [
        this,
        method,
        url{move(url)},
        hfields{move(hfields)},
        cb{move(cb)},
        opt{move(options)}
      ]
        (net::ip4::Addr ip)
      {
        // Host resolved
        if (ip != 0)
        {
          // setup request with method and headers
          auto req = create_request(method);
          *req << hfields;

          // Set Host and URI path
          populate_from_url(*req, url);

          // Default to port 80 if non given
          const uint16_t port = (url.port() != 0xFFFF) ? url.port() : 80;

          send(move(req), {ip, port}, move(cb), move(opt));
        }
        else
        {
          cb({Error::RESOLVE_HOST}, nullptr, Connection::empty());
        }
      }));
    }
  }

  void Client::request(Method method, Host host, std::string path, Header_set hfields, Response_handler cb, Options options)
  {
    using namespace std;
    // setup request with method and headers
    auto req = create_request(method);
    *req << hfields;

    //set uri (default "/")
    req->set_uri((!path.empty()) ? URI{move(path)} : URI{"/"});

    send(move(req), move(host), move(cb), move(options));
  }

  void Client::request(Method method, URI url, Header_set hfields, std::string data, Response_handler cb, Options options)
  {
    using namespace std;
    tcp_.stack().resolve(
      url.host().to_string(),
      ResolveCallback::make_packed(
      [
        this,
        method,
        url{move(url)},
        hfields{move(hfields)},
        data{move(data)},
        cb{move(cb)},
        opt{move(options)}
      ] (auto ip)
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

          send(move(req), {ip, port}, move(cb), move(opt));
        }
        else
        {
          cb({Error::RESOLVE_HOST}, nullptr, Connection::empty());
        }
      })
    );
  }

  void Client::request(Method method, Host host, std::string path, Header_set hfields, const std::string& data, Response_handler cb, Options options)
  {
    using namespace std;
    // setup request with method and headers
    auto req = create_request(method);
    *req << hfields;

    // set uri (default "/")
    req->set_uri((!path.empty()) ? URI{move(path)} : URI{"/"});

    // Add data and content length
    add_data(*req, data);

    send(move(req), move(host), move(cb), move(options));
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
    // Set uri path (default "/")
    req.set_uri((!url.path().empty()) ? URI{url.path()} : URI{"/"});

    // Set Host: host(:port)
    const auto port = url.port();
    req.header().set_field(header::Host,
      (port != 0xFFFF and port != 80) ?
      url.host().to_string() + ":" + std::to_string(port)
      : url.host().to_string()); // to_string madness
  }

  void Client::resolve(const std::string& host, ResolveCallback cb)
  {
    static auto&& stack = tcp_.stack();
    stack.resolve(host, cb);
  }

  Client_connection& Client::get_connection(const Host host)
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
    cset.push_back(std::make_unique<Client_connection>(*this, std::make_unique<Connection::Stream>(tcp_.connect(host))));
    return *cset.back();
  }

  void Client::close(Client_connection& c)
  {
    debug("<http::Client> Closing %u:%s %p\n", c.local_port(), c.peer().to_string().c_str(), &c);
    auto& cset = conns_.at(c.peer());

    cset.erase(std::remove_if(cset.begin(), cset.end(),
    [port = c.local_port()] (const std::unique_ptr<Client_connection>& conn)->bool
    {
      return conn->local_port() == port;
    }));
  }

}
