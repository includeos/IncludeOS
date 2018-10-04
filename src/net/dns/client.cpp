// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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
      socket_(nullptr),
      cache_ttl_{DEFAULT_CACHE_TTL},
      flush_timer_{{this, &DNSClient::flush_expired}}
  {
  }

  void DNSClient::bind_socket()
  {
    Expects(socket_ == nullptr);
    socket_ = &stack_.udp().bind();
    // Parse received data on this socket as Responses
    socket_->on_read({this, &DNSClient::receive_response});
  }

  void DNSClient::resolve(Address dns_server,
                          const Hostname& hname,
                          Resolve_handler func,
                          Timer::duration_t timeout, bool force)
  {
    if(UNLIKELY(socket_ == nullptr))
      bind_socket();

    auto hostname = hname; // fixme: unecessary copy
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
    // create DNS request
    DNS::Request request;
    std::array<char, 256> buf{};
    size_t len  = request.create(buf.data(), hostname);

    auto key = request.get_id();

    // store the request for later match
    requests_.emplace(std::piecewise_construct,
      std::forward_as_tuple(key),
      std::forward_as_tuple(std::move(request), std::move(func), timeout));

    // send request to DNS server
    socket_->sendto(dns_server, DNS::DNS_SERVICE_PORT, buf.data(), len, nullptr,
      [this, key] (const Error& err)
    {
      // If an error is not received, this will never execute (Error is just erased from the map
      // without calling the callback)

      // Find the request and remove it since an error occurred
      auto it = requests_.find(key);
      if (it != requests_.end()) {
        it->second.callback(IP4::ADDR_ANY, err);
        requests_.erase(it);
      }
    });
  }

  void DNSClient::flush_cache()
  {
    cache_.clear();

    flush_timer_.stop();
  }

  void DNSClient::receive_response(Address, UDP::port_t, const char* data, size_t len)
  {
    if(UNLIKELY(len < sizeof(DNS::header)))
      return; // no point in even bothering

    const auto& reply = *(DNS::header*) data;
    // match the transactions id on the reply with the ones in our map
    auto it = requests_.find(ntohs(reply.id));
    // if this is match
    if(it != requests_.end())
    {
      auto& req = it->second;
      // TODO: do some necessary validation ... (truncate etc?)

      auto& dns_req = req.request;
      // parse request
      dns_req.parseResponse(data, len);

      // cache the response for 60 seconds
      if(cache_ttl_ > std::chrono::seconds::zero())
      {
        const auto ip = dns_req.getFirstIP4();

        // online cache if ip was resolved
        if(ip != 0)
          add_cache_entry(dns_req.hostname(), ip, cache_ttl_);
      }

      // fire onResolve event
      req.finish();

      // the request is finished, removed it from our map
      requests_.erase(it);
    }
    else
    {
      debug("<DNSClient::receive_response> Cannot find matching DNS Request with transid=%u\n", ntohs(reply.id));
    }
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
    const auto before = cache_.size();

    // which key that has expired
    std::vector<const std::string*> expired;
    expired.reserve(before);

    const auto now = timestamp();
    // gather all expired entries
    for(auto& ent : cache_)
    {
      if(ent.second.expires <= now)
        expired.push_back(&ent.first);
    }

    // remove all expired from cache
    for(auto* exp : expired)
      cache_.erase(*exp);

    debug("<DNSClient> Flushed %u expired entries.\n", before - cache_.size());

    if(not cache_.empty())
      flush_timer_.start(DEFAULT_FLUSH_INTERVAL);
  }

  DNSClient::~DNSClient()
  {
    if(socket_ != nullptr)
    {
      socket_->close();
      socket_ = nullptr;
    }
  }
}
