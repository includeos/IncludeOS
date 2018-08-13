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

namespace net
{
  Timer::duration_t DNSClient::DEFAULT_RESOLVE_TIMEOUT{std::chrono::seconds(5)};
  Timer::duration_t DNSClient::DEFAULT_FLUSH_INTERVAL{std::chrono::seconds(30)};
  std::chrono::seconds DNSClient::DEFAULT_CACHE_TTL{std::chrono::seconds(60)};

  DNSClient::DNSClient(Stack& stack)
    : stack_{stack},
      cache_ttl_{DEFAULT_CACHE_TTL},
      flush_timer_{{this, &DNSClient::flush_expired}}
  {
  }

  void DNSClient::resolve(Address dns_server,
                          Hostname hostname,
                          Resolve_handler func,
                          Timer::duration_t timeout, bool force)
  {
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
        func(it->second.address.v4(), err);
        return;
      }
    }
    // Make sure we actually can bind to a socket
    auto& socket = stack_.udp().bind();

    // Create our query
    dns::Query query{std::move(hostname), dns::Record_type::A};

    // store the request for later match
    auto emp = requests_.emplace(std::piecewise_construct,
      std::forward_as_tuple(query.id),
      std::forward_as_tuple(*this, socket, std::move(query), std::move(func)));

    Ensures(emp.second && "Unable to insert");
    auto& req = emp.first->second;
    req.resolve(dns_server, timeout);
  }

  DNSClient::Request::Request(DNSClient& cli, udp::Socket& sock,
                              dns::Query q, Resolve_handler cb)
    : client{cli},
      query{std::move(q)},
      response{nullptr},
      socket{sock},
      callback{std::move(cb)},
      timer({this, &Request::finish})
  {
    socket.on_read({this, &DNSClient::Request::parse_response});
  }

  void DNSClient::Request::resolve(net::Addr server, Timer::duration_t timeout)
  {
    std::array<char, 256> buf;
    size_t len = query.write(buf.data());

    socket.sendto(server, dns::SERVICE_PORT, buf.data(), len, nullptr,
      {this, &DNSClient::Request::handle_error});

    timer.start(timeout);
  }

  void DNSClient::Request::parse_response(Addr, UDP::port_t, const char* data, size_t len)
  {
    const auto& reply = *(dns::Header*) data;

    Error err;
    // this is a response to our query
    if(query.id == ntohs(reply.id))
    {
      // TODO: Validate

      response.reset(new dns::Response(data));

      // TODO: Cache
      /*if(client.cache_ttl_ > std::chrono::seconds::zero())
      {
        add_cache_entry(...);
      }*/
    }

    finish(err);
  }

  void DNSClient::Request::finish(const Error& err)
  {
    ip4::Addr addr = (response) ? response->get_first_ipv4() : 0;
    callback(addr, err);

    auto erased = client.requests_.erase(query.id);
    Ensures(erased == 1);
  }

  void DNSClient::Request::handle_error(const Error& err)
  {
    // This will call the user callback - do we want that?
    finish(err);
  }

  void DNSClient::flush_cache()
  {
    cache_.clear();

    flush_timer_.stop();
  }

  void DNSClient::add_cache_entry(const Hostname& hostname, Address addr, std::chrono::seconds ttl)
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

  void DNSClient::flush_expired()
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
