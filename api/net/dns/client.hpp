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

#ifndef NET_DNS_CLIENT_HPP
#define NET_DNS_CLIENT_HPP

#include <net/dns/dns.hpp>
#include <net/ip4/ip4.hpp>
#include <net/udp/socket.hpp>
#include <util/timer.hpp>
#include <map>
#include <unordered_map>
#include "query.hpp"
#include "response.hpp"

namespace net
{
  class Inet;
}
namespace net::dns {
  /**
   * @brief      A simple DNS client which is able to resolve hostnames
   *             and locally cache them.
   *
   * @note       A entry can stay longer than TTL due to flush timer granularity.
   *             Max_TTL = TTL + FLUSH_INTERVAL (90s default)
   */
  class Client
  {
  public:
    using Stack           = Inet;
    using Resolve_handler = delegate<void(dns::Response_ptr, const Error& err)>;
    using Address         = net::Addr;
    using Hostname        = std::string;
    using timestamp_t     = RTC::timestamp_t;
    /**
     * @brief      A simple cache entry containing the resolved IP address
     *             and a timestamp when it expires (based on uptime)
     */
    struct Cache_entry
    {
      Address     address;
      timestamp_t expires;

      Cache_entry(Address addr, const timestamp_t exp) noexcept
        : address{std::move(addr)}, expires{exp}
      {}
    };
    using Cache           = std::unordered_map<Hostname, Cache_entry>;

    static Timer::duration_t DEFAULT_RESOLVE_TIMEOUT; // 5s, client.cpp
    static Timer::duration_t DEFAULT_FLUSH_INTERVAL; // 60s, client.cpp
    static std::chrono::seconds DEFAULT_CACHE_TTL; // 60s, client.cpp

    /**
     * @brief      Construct a DNS client on a given interface (stack),
     *             binding to a UDP socket to be used for DNS request/responses.
     *
     * @param      stack   The stack
     */
    Client(Stack& stack);

    /**
     * @brief      Resolve a hostname for an IP4 address with a timeout duration
     *             and an option whether to force the request, disabling cache lookup.
     *             Invokes the resolve handler with the address and a error (if any happend).
     *             The address can be 0 (INADDR_ANY) which means that
     *             1) either the hostname was not found or
     *             2) the request timed out.
     *
     * @param[in]  dns_server  The dns server where to send the request
     * @param[in]  hostname    The hostname to resolve
     * @param[in]  handler     The resolve handler
     * @param[in]  timeout     The time before the request times out
     * @param[in]  force       Wether to force the resolve, ignoring the cache
     */
    void resolve(Address            dns_server,
                 Hostname           hostname,
                 Resolve_handler    handler,
                 Timer::duration_t  timeout,
                 bool               force = false);

    /**
     * @brief      Resolve a hostname with default timeout.
     */
    void resolve(Address            dns_server,
                 Hostname           hostname,
                 Resolve_handler    handler,
                 bool               force = false)
    {
      resolve(dns_server, std::move(hostname), std::move(handler), DEFAULT_RESOLVE_TIMEOUT, force);
    }

    /**
     * @brief      Flush the cache, removing all entries.
     */
    void flush_cache();

    /**
     * @brief      Returns a readable reference to the local cache.
     *
     * @return     A Cache reference to the local cache
     */
    const Cache& cache() const
    { return cache_; }

    /**
     * @brief      Returns the time to live value of an entry in the cache.
     *
     * @return     Time to live in seconds
     */
    auto cache_ttl()
    { return cache_ttl_; }

    /**
     * @brief      Sets the time to live for a cache entry
     *             A value of zero means caching is disabled.
     *
     * @param[in]  ttl   The ttl in seconds
     */
    void set_cache_ttl(std::chrono::seconds ttl)
    { cache_ttl_ = ttl; }

    /**
     * @brief      Disables caching
     */
    void disable_cache()
    { set_cache_ttl(std::chrono::seconds::zero()); }

    /**
     * @brief      Enables caching
     *
     * @param[in]  ttl   The ttl for a cache entry (optional)
     */
    void enable_cache(std::chrono::seconds ttl = DEFAULT_CACHE_TTL)
    { set_cache_ttl(ttl); }

    static bool is_FQDN(const std::string& hostname)
    { return hostname.find('.') != std::string::npos; }

  private:
    Stack&                stack_;
    Cache                 cache_;
    std::chrono::seconds  cache_ttl_;
    Timer                 flush_timer_;

    /**
     * @brief      Receive a UDP message with a (hopefully) DNS response
     *             to one of our requests.
     *
     * @param[in]  <unnamed>  From whom address
     * @param[in]  <unnamed>  From whom port
     * @param[in]  data       The raw data containing the msg
     * @param[in]  <unnamed>  Size of the data
     */
    void receive_response(Address, udp::port_t, const char* data, size_t);

    /**
     * @brief      Adds a cache entry.
     *
     * @param[in]  hostname  The hostname
     * @param[in]  addr      The address
     * @param[in]  ttl       The ttl
     */
    void add_cache_entry(const Hostname& hostname, Address addr, std::chrono::seconds ttl);

    /**
     * @brief      Flush all expired cache entries.
     *             Called internally from the flush timer.
     */
    void flush_expired();

    /**
     * @brief      Returns a timestamp used when calculating TTL.
     *
     * @return     A timestamp in seconds.
     */
    timestamp_t timestamp() const
    { return RTC::time_since_boot(); }

    /**
     * @brief      An internal client request. Contains the DNS request itself,
     *             the callback to be called when resolved (or timedout) and
     *             a timeout timer.
     */
    struct Request
    {
      Client&      client;

      dns::Query      query;
      using Response_ptr = std::unique_ptr<dns::Response>;
      Response_ptr    response;

      udp::Socket&    socket;

      Resolve_handler callback;
      Timer           timer;

      Request(Client& cli, udp::Socket& sock, dns::Query q, Resolve_handler cb);

      void resolve(Address server, Timer::duration_t timeout);

      ~Request();

    private:

      void parse_response(Addr, udp::port_t, const char* data, size_t len);

      void handle_error(const Error& err);

      /**
       * @brief      Finish the request with a no error,
       *             invoking the resolve handler (callback)
       */
      void finish(const Error& err);

      void timeout();

    }; // < struct Request
       //
    using Requests = std::unordered_map<dns::id_t, Request>;
    /** Pending requests (not yet resolved) */
    Requests requests_;
  };
}

#endif
