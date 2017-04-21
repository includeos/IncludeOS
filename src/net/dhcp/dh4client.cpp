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

//#define DEBUG
#include <info>
#define MYINFO(X,...) INFO("DHCPv4",X,##__VA_ARGS__)

#include <net/dhcp/dh4client.hpp>
#include <net/dhcp/message.hpp>
#include <debug>

#include <random>

namespace net {

  using namespace dhcp;

  DHClient::DHClient(Stack& inet)
    : stack(inet), xid(0), console_spam(true), in_progress(false)
  {
    this->on_config(
    [this] (bool timeout)
    {
      if (console_spam) {
        if (timeout)
          INFO("DHCPv4", "Negotiation timed out");
        else
          INFO("DHCPv4", "Config complete");
      }
    });
  }

  auto generate_xid()
  {
    // create a random session ID
    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(std::numeric_limits<uint32_t>::min(), std::numeric_limits<uint32_t>::max());
    return dis(gen);
  }

  void DHClient::on_config(config_func handler)
  {
    assert(handler);
    config_handlers_.push_back(handler);
  }

  void DHClient::negotiate(uint32_t timeout_secs)
  {
    // Allow multiple calls to negotiate without restarting the process
    if (in_progress) return;
    in_progress = true;

    // set timeout handler
    using namespace std::chrono;
    this->timeout = Timers::oneshot(seconds(timeout_secs),
    [this] (int) {
      // reset session ID
      this->xid = 0;
      this->in_progress = false;

      // call on_config with timeout = true
      for(auto handler : this->config_handlers_)
          handler(true);
    });

    // create a random session ID
    this->xid = generate_xid();

    if (console_spam)
      MYINFO("Negotiating IP-address (xid=%u)", xid);

    // create DHCP discover packet
    uint8_t buffer[Message::size()];
    auto msg = Message_view::create(&buffer[0], op_code::BOOTREQUEST, message_type::DISCOVER);
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
      option::DOMAIN_NAME_SERVERS
    });
    // END
    msg.end();

    ////////////////////////////////////////////////////////
    auto& socket = stack.udp().bind(DHCP_CLIENT_PORT);
    /// broadcast our DHCP plea as 0.0.0.0:67
    socket.bcast(IP4::ADDR_ANY, DHCP_SERVER_PORT, buffer, sizeof(buffer));

    socket.on_read(
    [this, &socket] (IP4::addr addr, UDP::port_t port,
                     const char* data, size_t len)
    {
      if (port == DHCP_SERVER_PORT)
      {
        // we have got a DHCP Offer
        MYINFO("Received possible DHCP OFFER from %s",
               addr.str().c_str());
        this->offer(socket, data, len);
      }
    });
  }

  void DHClient::offer(UDPSocket& sock, const char* data, size_t)
  {
    const auto msg = Message_view::open(reinterpret_cast<uint8_t*>(const_cast<char*>(data))); // lol, temp

    // silently ignore transactions not our own
    if (msg.xid() != this->xid) return;

    //const dhcp_option_t* server_id {nullptr};

    // check if the BOOTP message is a DHCP OFFER
    const auto* msg_opt = msg.find_option<option::message>();
    if(msg_opt != nullptr)
    {
      // ignore when not a DHCP Offer
      if (UNLIKELY(msg_opt->type() != message_type::OFFER)) return;

      // verify that the type is indeed DHCPOFFER
      debug("Got DHCP message type OFFER (%d)\n",
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

    // Remove any existing IP config to be able to receive on broadcast
    stack.reset_config();

    // we can accept the offer now by requesting the IP!
    this->request(sock, server_id);
  }

  void DHClient::request(UDPSocket& sock, const option::server_identifier* server_id)
  {
    // form a response
    uint8_t buffer[Message::size()];

    auto msg = Message_view::create(&buffer[0], op_code::BOOTREQUEST, message_type::REQUEST);

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
      MYINFO("Server IP set to %s", addr->to_string().c_str());
    }
    else
    {
      msg.add_option<option::server_identifier>(&this->router);
      MYINFO("Server IP set to gateway (%s)", this->router.to_string().c_str());
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

    // set our onRead function to point to a hopeful DHCP ACK!
    sock.on_read(
    [this] (IP4::addr, UDP::port_t port,
          const char* data, size_t len)
    {
      if (port == DHCP_SERVER_PORT)
      {
        // we have hopefully got a DHCP Ack
        debug("\tReceived DHCP ACK from %s:%d\n",
          addr.str().c_str(), DHCP_SERVER_PORT);
        this->acknowledge(data, len);
      }
    });

    // send our DHCP Request
    sock.bcast(IP4::ADDR_ANY, DHCP_SERVER_PORT, buffer, sizeof(buffer));
  }

  void DHClient::acknowledge(const char* data, size_t)
  {
    const auto msg = Message_view::open(reinterpret_cast<uint8_t*>(const_cast<char*>(data))); // lol, temp

    // silently ignore transactions not our own
    if (msg.xid() != this->xid) return;

    // check if the BOOTP message is a DHCP ACK
    const auto* msg_opt = msg.find_option<option::message>();
    if(msg_opt != nullptr)
    {
      // ignore when not a DHCP Ack
      if (UNLIKELY(msg_opt->type() != message_type::ACK)) return;

      // verify that the type is indeed DHCPOFFER
      debug("\tFound DHCP message type %d  (DHCP Ack = %d)\n",
            static_cast<uint8_t>(msg_opt->type()), message_type::ACK);
    }
    // ignore message when DHCP message type is missing
    else return;

    debug("Server acknowledged our request!");
    debug("IP ADDRESS: \t%s", this->ipaddr.str().c_str());
    debug("SUBNET MASK: \t%s", this->netmask.str().c_str());
    debug("LEASE TIME: \t%u mins", this->lease_time / 60);
    debug("GATEWAY: \t%s", this->router.str().c_str());
    debug("DNS SERVER: \t%s", this->dns_server.str().c_str());

    MYINFO("DHCP configuration succeeded");

    // configure our network stack
    stack.network_config(this->ipaddr, this->netmask,
                         this->router, this->dns_server);
    // stop timeout from happening
    Timers::stop(timeout);

    in_progress = false;

    // run some post-DHCP event to release the hounds
    for (auto handler : config_handlers_)
      handler(false);
  }

}
