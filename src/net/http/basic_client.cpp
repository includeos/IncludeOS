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

#include <net/http/basic_client.hpp>

namespace http {

  const Basic_client::timeout_duration Basic_client::DEFAULT_TIMEOUT{std::chrono::seconds(5)};
  int Basic_client::default_follow_redirect{0};

  Basic_client::Basic_client(TCP& tcp, Request_handler on_send)
    : Basic_client(tcp, std::move(on_send), false)
  {
  }

  Basic_client::Basic_client(TCP& tcp, Request_handler on_send, const bool https_supported)
    : tcp_(tcp),
      on_send_{std::move(on_send)},
      supports_https(https_supported)
  {
  }

  Request_ptr Basic_client::create_request(Method method) const
  {
    auto req = std::make_unique<Request>();
    req->set_method(method);

    auto& header = req->header();
    header.set_field(header::User_Agent, "IncludeOS/0.12");
    set_connection_header(*req);

    return req;
  }

  void Basic_client::send(Request_ptr req, Host host, Response_handler cb,
                    const bool secure, Options options)
  {
    Expects(cb != nullptr);
    using namespace std;
    auto& conn = (not secure) ?
      get_connection(host) : get_secure_connection(host);

    auto&& header = req->header();

    // Set Host if not already set
    if(! header.has_field(header::Host))
      header.set_field(header::Host, host.to_string());

    // Set Origin if not already set
    if(! header.has_field(header::Origin))
      header.set_field(header::Origin, origin());

    debug("<http::Basic_client> Sending Request:\n%s\n", req->to_string().c_str());

    if(on_send_)
      on_send_(*req, options, host);

    conn.send(move(req), move(cb), options.follow_redirect, options.timeout);
  }

  void Basic_client::send(Request_ptr req, URI url, Response_handler cb, Options options)
  {
    Expects(url.is_valid() && "Invalid URI (missing scheme?)");
    // find out if this is a secured request or not
    const bool secure = url.scheme_is_secure();
    validate_secure(secure);

    using namespace std;

    // Default to port 80 if non given
    const uint16_t port = (url.port() != 0xFFFF) ? url.port() : 80;

    if (url.host_is_ip4())
    {
      std::string host(url.host());
      auto ip = net::ip4::Addr(host);

      send(move(req), {ip, port}, move(cb), secure, move(options));
    }
    else
    {
      tcp_.stack().resolve(std::string(url.host()),
      ResolveCallback::make_packed(
      [
        this,
        request = move(req),
        url{move(url)},
        cb{move(cb)},
        opt{move(options)},
        secure,
        port
      ]
        (net::dns::Response_ptr res, const net::Error&) mutable
      {
        auto addr = res->get_first_addr();
        if(UNLIKELY(addr == net::Addr::addr_any))
        {
          cb({Error::RESOLVE_HOST}, nullptr, Connection::empty());
          return;
        }
        // Host resolved
        send(move(request), {addr, port}, move(cb), secure, move(opt));
      }));
    }
  }

  void Basic_client::request(Method method, URI url, Header_set hfields,
                       Response_handler cb, Options options)
  {
    Expects(url.is_valid() && "Invalid URI (missing scheme?)");
    Expects(cb != nullptr);

    // find out if this is a secured request or not
    const bool secure = url.scheme_is_secure();
    validate_secure(secure);

    using namespace std;

    if (url.host_is_ip4())
    {
      std::string host(url.host());
      auto ip = net::ip4::Addr(host);
      // setup request with method and headers
      auto req = create_request(method);
      *req << hfields;

      // Set Host and URI path
      populate_from_url(*req, url);

      // Default to port 80 if non given
      const uint16_t port = (url.port() != 0xFFFF) ? url.port() : 80;

      send(move(req), {ip, port}, move(cb), secure, move(options));
    }
    else
    {
      tcp_.stack().resolve(std::string(url.host()),
      ResolveCallback::make_packed(
      [
        this,
        method,
        url{move(url)},
        hfields{move(hfields)},
        cb{move(cb)},
        opt{move(options)},
        secure
      ]
        (net::dns::Response_ptr res, const net::Error& err)
      {
        // Host resolved
        if (not err)
        {
          auto addr = res->get_first_addr();
          if(UNLIKELY(addr == net::Addr::addr_any))
          {
            cb({Error::RESOLVE_HOST}, nullptr, Connection::empty());
            return;
          }
          // setup request with method and headers
          auto req = create_request(method);
          *req << hfields;

          // Set Host and URI path
          populate_from_url(*req, url);

          // Default to port 80 if non given
          const uint16_t port = (url.port() != 0xFFFF) ? url.port() : 80;

          send(move(req), {addr, port}, move(cb), secure, move(opt));
        }
        else
        {
          cb({Error::RESOLVE_HOST}, nullptr, Connection::empty());
        }
      }));
    }
  }

  void Basic_client::request(Method method, Host host, std::string path,
                       Header_set hfields, Response_handler cb,
                      const bool secure, Options options)
  {
    validate_secure(secure);

    using namespace std;
    // setup request with method and headers
    auto req = create_request(method);
    *req << hfields;

    //set uri (default "/")
    req->set_uri((!path.empty()) ? URI{move(path)} : URI{"/"});

    send(move(req), move(host), move(cb), secure, move(options));
  }

  void Basic_client::request(Method method, URI url, Header_set hfields,
                       std::string data, Response_handler cb,
                       Options options)
  {
    Expects(url.is_valid() && "Invalid URI (missing scheme?)");
    // find out if this is a secured request or not
    const bool secure = url.scheme_is_secure();
    validate_secure(secure);

    using namespace std;
    if (url.host_is_ip4())
    {
      std::string host(url.host());
      auto ip = net::ip4::Addr(host);
      // setup request with method and headers
      auto req = create_request(method);
      *req << hfields;

      // Set Host and URI path
      populate_from_url(*req, url);

      // Add data and content length
      this->add_data(*req, data);

      // Default to port 80 if non given
      const uint16_t port = (url.port() != 0xFFFF) ? url.port() : 80;

      send(move(req), {ip, port}, move(cb), secure, move(options));
    }
    else
    {
      tcp_.stack().resolve(
        std::string(url.host()),
        ResolveCallback::make_packed(
        [
          this,
          method,
          url{move(url)},
          hfields{move(hfields)},
          data{move(data)},
          cb{move(cb)},
          opt{move(options)},
          secure
        ]
          (net::dns::Response_ptr res, const net::Error& err)
        {
          // Host resolved
          if (not err)
          {
            auto addr = res->get_first_addr();
            if(UNLIKELY(addr == net::Addr::addr_any))
            {
              cb({Error::RESOLVE_HOST}, nullptr, Connection::empty());
              return;
            }
            // setup request with method and headers
            auto req = this->create_request(method);
            *req << hfields;

            // Set Host & path from url
            this->populate_from_url(*req, url);

            // Add data and content length
            this->add_data(*req, data);

            // Default to port 80 if non given
            const uint16_t port = (url.port() != 0xFFFF) ? url.port() : 80;

            this->send(move(req), {addr, port}, move(cb), secure, move(opt));
          }
          else
          {
            cb({Error::RESOLVE_HOST}, nullptr, Connection::empty());
          }
        })
      );
    }
  }

  void Basic_client::request(Method method, Host host, std::string path,
                       Header_set hfields, const std::string& data,
                       Response_handler cb, const bool secure, Options options)
  {
    validate_secure(secure);

    using namespace std;
    // setup request with method and headers
    auto req = create_request(method);
    *req << hfields;

    // set uri (default "/")
    req->set_uri((!path.empty()) ? URI{move(path)} : URI{"/"});

    // Add data and content length
    add_data(*req, data);

    send(move(req), move(host), move(cb), secure, move(options));
  }

  void Basic_client::add_data(Request& req, const std::string& data)
  {
    auto& header = req.header();
    if(!header.has_field(header::Content_Type))
      header.set_field(header::Content_Type, "text/plain");

    // Set Content-Length to be equal data length
    req.header().set_field(header::Content_Length, std::to_string(data.size()));

    // Add data
    req.add_body(data);
  }

  void Basic_client::populate_from_url(Request& req, const URI& url)
  {
    Expects(url.is_valid() && "Invalid URI (missing scheme?)");
    // Set uri path (default "/")
    req.set_uri((!url.path().empty()) ? URI{url.path()} : URI{"/"});

    // Set Host: host(:port)
    const auto port = url.port();
    req.header().set_field(header::Host,
      (port != 0xFFFF and port != 80) ?
      std::string(url.host()) + ":" + std::to_string(port)
      : std::string(url.host())); // to_string madness
  }

  void Basic_client::resolve(const std::string& host, ResolveCallback cb)
  {
    static auto&& stack = tcp_.stack();
    stack.resolve(host, cb);
  }

  Client_connection& Basic_client::get_connection(const Host host)
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
    cset.push_back(std::make_unique<Client_connection>(
      *this, std::make_unique<net::tcp::Stream>(tcp_.connect(host)))
    );
    return *cset.back();
  }

  Client_connection& Basic_client::get_secure_connection(const Host)
  {
    throw Client_error{"Secured connections not supported (use the HTTPS Client)."};
  }

  void Basic_client::close(Client_connection& c)
  {
    debug("<http::Basic_client> Closing %u:%s %p\n", c.local_port(), c.peer().to_string().c_str(), &c);
    auto& cset = conns_.at(c.peer());

    cset.erase(std::remove_if(cset.begin(), cset.end(),
    [port = c.local_port()] (const std::unique_ptr<Client_connection>& conn)->bool
    {
      return conn->local_port() == port;
    }));
  }

}
