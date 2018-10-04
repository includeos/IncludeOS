// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

//#define DHCP_DEBUG 1
#ifdef DHCP_DEBUG
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif

#include <info>
#define MYINFO(X,...) INFO("DHCPv4",X,##__VA_ARGS__)

#include <net/dhcp/dh4client.hpp>
#include <net/inet>
#include <net/dhcp/message.hpp>
#include <cstdlib>
#include <debug>

namespace net {

  using namespace dhcp;

  DHClient::DHClient(Stack& inet)
    : stack(inet),
      domain_name{},
      timeout_timer_{{this, &DHClient::restart_negotation}}
  {
    // default timed out handler spams logs
    this->on_config(
    [this] (bool timed_out)
    {
      if (timed_out)
        MYINFO("Negotiation timed out (%s)", this->stack.ifname().c_str());
      else
        MYINFO("Configuration complete (%s)", this->stack.ifname().c_str());
    });
  }

  void DHClient::on_config(config_func handler)
  {
    assert(handler);
    config_handlers_.push_back(handler);
  }

  void DHClient::restart_negotation()
  {
    tries++;
    // if timeout is supplied
    if(timeout != std::chrono::seconds::zero())
    {
      // calculate if we should retry
      const bool retry = (timeout / (tries * RETRY_FREQUENCY)) >= 1;

      if(retry)
      {
        timeout_timer_.start(RETRY_FREQUENCY);
        send_first();
        return;
      }
      else
      {
        end_negotiation(true);
        return;
      }
    }

    static const int FAST_TRIES = 10;
    // never timeout
    if(tries <= FAST_TRIES)
    {
      // do fast retry
      timeout_timer_.start(RETRY_FREQUENCY);
    }
    else
    {
      if(UNLIKELY(tries == FAST_TRIES+1)) {
        MYINFO("No reply for %i tries, retrying every %lli second",
          FAST_TRIES, RETRY_FREQUENCY_SLOW.count());
      }

      // fallback to slow retry
      timeout_timer_.start(RETRY_FREQUENCY_SLOW);
    }

    send_first();
  }

  void DHClient::end_negotiation(bool timed_out)
  {
    // wind down
    this->xid      = 0;
    timeout_timer_.stop();
    this->progress = 0;
    this->tries  = 0;
    // close UDP socket
    Expects(this->socket != nullptr);
    this->socket->close();
    this->socket = nullptr;
    // call on_config with timeout = true
    for(auto& handler : this->config_handlers_)
        handler(timed_out);

    if(timeout_timer_.is_running()) timeout_timer_.stop();
  }

  void DHClient::negotiate(std::chrono::seconds timeout)
  {
    // Allow multiple calls to negotiate without restarting the process
    if (this->xid != 0) return;
    this->tries = 0;
    this->progress = 0;

    // calculate progress timeout
    using namespace std::chrono;
    this->timeout = timeout;

    // generate a new session ID
    this->xid  = (rand() & 0xffff);
    this->xid |= (rand() & 0xffff) << 16;
    assert(this->xid != 0);

    PRINT("Negotiating IP-address for %s (xid=%u)\n", stack.ifname().c_str(), xid);

    assert(this->socket == nullptr);
    this->socket = &stack.udp().bind(DHCP_CLIENT_PORT);

    restart_negotation();
  }

  void DHClient::send_first()
  {
    // create DHCP discover packet
    uint8_t buffer[Message::size()];
    Message_writer msg{&buffer[0], op_code::BOOTREQUEST, message_type::DISCOVER};
    msg.set_hw_addr(htype::ETHER, sizeof(MAC::Addr)); // eth dependency
    msg.set_xid(this->xid);
    msg.set_flag(flag::BOOTP_BROADCAST);

    MAC::Addr link_addr = stack.link_addr();
    msg.set_chaddr(&link_addr);

     // DHCP client identifier
    msg.add_option<option::client_identifier>(htype::ETHER, &link_addr);

    // DHCP Parameter Request Field
    msg.add_option<option::param_req_list>(std::vector<option::Code>{
      option::ROUTERS,
      option::SUBNET_MASK,
      option::DOMAIN_NAME_SERVERS,
      option::DOMAIN_NAME
    });
    // END
    msg.end();

    assert(socket);
    /// broadcast our DHCP plea as 0.0.0.0:67
    socket->bcast(IP4::ADDR_ANY, DHCP_SERVER_PORT, buffer, sizeof(buffer));
    socket->on_read(
    [this] (net::Addr addr, UDP::port_t port,
                     const char* data, size_t len)
    {
      if (port == DHCP_SERVER_PORT)
      {
        // we have got a DHCP Offer
        (void) addr;
        PRINT("Received possible DHCP OFFER from %s\n",
               addr.to_string().c_str());
        this->offer(data, len);
      }
    });
  }

  void DHClient::offer(const char* data, size_t)
  {
    const Message_reader msg{reinterpret_cast<const uint8_t*>(data)};

    // silently ignore transactions not our own
    if (msg.xid() != this->xid) return;

    //const dhcp_option_t* server_id {nullptr};

    // check if the BOOTP message is a DHCP OFFER
    const auto* msg_opt = msg.find_option<option::message_type>();
    if(msg_opt != nullptr)
    {
      // ignore when not a DHCP Offer
      if (UNLIKELY(msg_opt->type() != message_type::OFFER)) return;

      // verify that the type is indeed DHCPOFFER
      PRINT("Got DHCP message type OFFER (%d)\n",
            static_cast<uint8_t>(msg_opt->type()));
    }
    // ignore message when DHCP message type is missing
    else return;

    // the offered IP address:
    this->ipaddr = msg.yiaddr();

    // check for subnet mask
    const auto* subnet_opt = msg.find_option<option::subnet_mask>();
    if(subnet_opt != nullptr)
    {
      this->netmask = *(subnet_opt->addr<ip4::Addr>());
    }

    // check for lease time
    const auto* lease_opt = msg.find_option<option::lease_time>();
    if (lease_opt != nullptr)
    {
      this->lease_time = lease_opt->secs();
    }

    // Preserve DHCP server address
    const auto* server_id = msg.find_option<option::server_identifier>();

    // now validate the offer, checking for minimum information
    // routers
    const auto* routers_opt = msg.find_option<option::routers>();
    if (routers_opt != nullptr)
    {
      this->router = *(routers_opt->addr<ip4::Addr>());
    }
    // assume that the server we received the request from is the gateway
    else
    {
      if (server_id)
        this->router = *(server_id->addr<ip4::Addr>());

      // silently ignore when both ROUTER and SERVER_ID is missing
      else return;
    }

    // domain name servers
    const auto* dns_opt = msg.find_option<option::domain_name_servers>();
    if (dns_opt != nullptr)
    {
      this->dns_server = *(dns_opt->addr<ip4::Addr>());
    }
    else
    { // just try using ROUTER as DNS server
      this->dns_server = this->router;
    }

    // domain name
    const auto* dn_opt = msg.find_option<option::domain_name>();
    if (dn_opt != nullptr)
    {
      auto dname = dn_opt->name();
      if(not dname.empty())
      {
        this->domain_name = std::move(dname);
      }
      //printf("Found Domain name option: %s\n", dn_opt->name().c_str());
    }

    // Remove any existing IP config to be able to receive on broadcast
    stack.reset_config();

    this->progress++;
    // we can accept the offer now by requesting the IP!
    this->request(server_id);
  }

  void DHClient::request(const option::server_identifier* server_id)
  {
    // form a response
    uint8_t buffer[Message::size()];

    Message_writer msg{&buffer[0], op_code::BOOTREQUEST, message_type::REQUEST};

    msg.set_hw_addr(htype::ETHER, sizeof(MAC::Addr)); // eth dependency
    msg.set_xid(this->xid);
    msg.set_flag(flag::BOOTP_BROADCAST);

    MAC::Addr link_addr = stack.link_addr();
    msg.set_chaddr(&link_addr);

    // DHCP client identifier
    msg.add_option<option::client_identifier>(htype::ETHER, &link_addr);

    // DHCP server identifier
    if (server_id)
    {
      const auto* addr = server_id->addr<ip4::Addr>();
      msg.add_option<option::server_identifier>(addr);
      PRINT("Server IP set to %s\n", addr->to_string().c_str());
    }
    else
    {
      msg.add_option<option::server_identifier>(&this->router);
      PRINT("Server IP set to gateway (%s)\n", this->router.to_string().c_str());
    }

    // DHCP Requested Address
    msg.add_option<option::req_addr>(&this->ipaddr);

    // DHCP Parameter Request Field
    msg.add_option<option::param_req_list>(std::vector<option::Code>{
      option::ROUTERS,
      option::SUBNET_MASK,
      option::DOMAIN_NAME_SERVERS
    });
    // END
    msg.end();

    assert(this->socket);
    // set our onRead function to point to a hopeful DHCP ACK!
    socket->on_read(
    [this] (net::Addr addr, UDP::port_t port,
          const char* data, size_t len)
    {
      if (port == DHCP_SERVER_PORT)
      {
        (void) addr;
        // we have hopefully got a DHCP Ack
        PRINT("\tReceived DHCP ACK from %s:%d\n",
          addr.to_string().c_str(), DHCP_SERVER_PORT);
        this->acknowledge(data, len);
      }
    });
    socket->bcast(IP4::ADDR_ANY, DHCP_SERVER_PORT, buffer, sizeof(buffer));
    this->progress++;
  }

  void DHClient::acknowledge(const char* data, size_t)
  {
    const Message_reader msg{reinterpret_cast<const uint8_t*>(data)};

    // silently ignore transactions not our own
    if (msg.xid() != this->xid) return;

    // check if the BOOTP message is a DHCP ACK
    const auto* msg_opt = msg.find_option<option::message_type>();
    if(msg_opt != nullptr)
    {
      // ignore when not a DHCP Ack
      if (UNLIKELY(msg_opt->type() != message_type::ACK)) return;

      // verify that the type is indeed DHCPOFFER
      PRINT("\tFound DHCP message type %d  (DHCP Ack = %d)\n",
            static_cast<uint8_t>(msg_opt->type()), message_type::ACK);
    }
    // ignore message when DHCP message type is missing
    else return;

    PRINT("Server acknowledged our request!\n");
    PRINT("IP ADDRESS: \t%s\n", this->ipaddr.str().c_str());
    PRINT("SUBNET MASK: \t%s\n", this->netmask.str().c_str());
    PRINT("LEASE TIME: \t%u mins\n", this->lease_time / 60);
    PRINT("GATEWAY: \t%s\n", this->router.str().c_str());
    PRINT("DNS SERVER: \t%s\n", this->dns_server.str().c_str());

    // configure our network stack
    stack.network_config(this->ipaddr, this->netmask,
                         this->router, this->dns_server);
    if(not domain_name.empty())
    {
      PRINT("DOMAIN NAME: \t%s\n", this->domain_name.c_str());
      stack.set_domain_name(domain_name);
    }
    // did not time out!
    end_negotiation(false);
  }

}
