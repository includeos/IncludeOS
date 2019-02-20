// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2018 Oslo and Akershus University College of Applied Sciences
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

#include <net/dns/client.hpp>
#include <net/inet>

namespace net::dns
{
#ifdef LIBFUZZER_ENABLED
  Timer::duration_t Client::DEFAULT_RESOLVE_TIMEOUT{std::chrono::seconds(9999)};
  uint16_t g_last_xid = 0;
#else
  Timer::duration_t Client::DEFAULT_RESOLVE_TIMEOUT{std::chrono::seconds(5)};
#endif
  Timer::duration_t Client::DEFAULT_FLUSH_INTERVAL{std::chrono::seconds(30)};
  std::chrono::seconds Client::DEFAULT_CACHE_TTL{std::chrono::seconds(60)};

  Client::Client(Stack& stack)
    : stack_{stack},
      cache_ttl_{DEFAULT_CACHE_TTL},
      flush_timer_{{this, &Client::flush_expired}}
  {
  }

  void Client::resolve(Address dns_server,
                       Hostname hostname,
                       Resolve_handler func,
                       Timer::duration_t timeout, bool force)
  {
    Expects(not hostname.empty());
    if(not is_FQDN(hostname) and not stack_.domain_name().empty())
    {
      hostname.append(".").append(stack_.domain_name());
    }
    if(not force)
    {
      auto it = cache_.find(hostname);
      if(it != cache_.end())
      {
        Error err;
        func(nullptr, err); // fix
        return;
      }
    }
    // Make sure we actually can bind to a socket
    auto& socket = (dns_server.is_v6()) ? stack_.udp().bind6() : stack_.udp().bind();

    // Create our query
    Query query{std::move(hostname), (dns_server.is_v6() ? Record_type::AAAA : Record_type::A)};
#ifdef LIBFUZZER_ENABLED
    g_last_xid = query.id;
#endif

    // store the request for later match
    auto emp = requests_.emplace(std::piecewise_construct,
      std::forward_as_tuple(query.id),
      std::forward_as_tuple(*this, socket, std::move(query), std::move(func)));

    Ensures(emp.second && "Unable to insert");
    auto& req = emp.first->second;
    req.resolve(dns_server, timeout);
  }

  Client::Request::Request(Client& cli, udp::Socket& sock,
                              dns::Query q, Resolve_handler cb)
    : client{cli},
      query{std::move(q)},
      response{nullptr},
      socket{sock},
      callback{std::move(cb)},
      timer({this, &Request::timeout})
  {
    socket.on_read({this, &Client::Request::parse_response});
  }

  void Client::Request::resolve(net::Addr server, Timer::duration_t timeout)
  {
    std::array<char, 256> buf;
    size_t len = query.write(buf.data());

    socket.sendto(server, dns::SERVICE_PORT, buf.data(), len, nullptr,
      {this, &Client::Request::handle_error});

    timer.start(timeout);
  }

  void Client::Request::parse_response(Addr, UDP::port_t, const char* data, size_t len)
  {
    if(UNLIKELY(len < sizeof(dns::Header)))
      return;

    const auto& reply = *(dns::Header*) data;

    // this is a response to our query
    if(query.id == ntohs(reply.id))
    {
      auto res = std::make_unique<dns::Response>();
      // TODO: Validate
      res->parse(data, len);

      this->response = std::move(res);

      // TODO: Cache
      /*if(client.cache_ttl_ > std::chrono::seconds::zero())
      {
        add_cache_entry(...);
      }*/

      finish({});
    }
    else
    {
      debug("<dns::Client::Request> Cannot find matching DNS Request with transid=%u\n",
        ntohs(reply.id));
    }
  }

  void Client::Request::finish(const Error& err)
  {
    callback(std::move(response), err);

    auto erased = client.requests_.erase(query.id);
    Ensures(erased == 1);
  }

  void Client::Request::timeout()
  {
    finish({Error::Type::timeout, "Request timed out"});
  }

  void Client::Request::handle_error(const Error& err)
  {
    // This will call the user callback - do we want that?
    finish(err);
  }

  Client::Request::~Request()
  {
    socket.close();
  }

  void Client::flush_cache()
  {
    cache_.clear();

    flush_timer_.stop();
  }

  void Client::add_cache_entry(const Hostname& hostname, Address addr, std::chrono::seconds ttl)
  {
    // cache the address
    cache_.emplace(std::piecewise_construct,
      std::forward_as_tuple(hostname),
      std::forward_as_tuple(addr, timestamp() + ttl.count()));

    debug("<DNSClient> Cache entry added: [%s] %s (%lld)\n",
      hostname.c_str(), addr.to_string().c_str(), ttl.count());

    // start the timer if not already active
    if(not flush_timer_.is_running())
      flush_timer_.start(DEFAULT_FLUSH_INTERVAL);
  }

  void Client::flush_expired()
  {
    const auto now = timestamp();
    for(auto it = cache_.begin(); it != cache_.end();)
    {
      if(it->second.expires > now)
        it++;
      else
        it = cache_.erase(it);
    }

    if(not cache_.empty())
      flush_timer_.start(DEFAULT_FLUSH_INTERVAL);
  }

}
