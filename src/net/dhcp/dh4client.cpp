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
#include <net/dhcp/dhcp4.hpp>
#include <debug>

#include <random>

namespace net
{
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
    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(std::numeric_limits<uint32_t>::min(), std::numeric_limits<uint32_t>::max());
    this->xid = dis(gen);

    if (console_spam)
      MYINFO("Negotiating IP-address (xid=%u)", xid);

    // create DHCP discover packet
    char packet[PACKET_SIZE];

    dhcp_packet_t* dhcp = (dhcp_packet_t*) packet;
    dhcp->op    = BOOTREQUEST;
    dhcp->htype = HTYPE_ETHER;
    dhcp->hlen  = ETH_ALEN;
    dhcp->hops  = 0;
    dhcp->xid   = htonl(this->xid);
    dhcp->secs  = 0;
    dhcp->flags = BOOTP_BROADCAST;
    dhcp->ciaddr = IP4::ADDR_ANY;
    dhcp->yiaddr = IP4::ADDR_ANY;
    dhcp->siaddr = IP4::ADDR_ANY;
    dhcp->giaddr = IP4::ADDR_ANY;

    MAC::Addr link_addr = stack.link_addr();

    // copy our hardware address to chaddr field
    memset(dhcp->chaddr, 0, dhcp_packet_t::CHADDR_LEN);
    memcpy(dhcp->chaddr, &link_addr, ETH_ALEN);
    // zero server, file and options
    memset(dhcp->sname, 0, dhcp_packet_t::SNAME_LEN + dhcp_packet_t::FILE_LEN);

    dhcp->magic[0] =  99;
    dhcp->magic[1] = 130;
    dhcp->magic[2] =  83;
    dhcp->magic[3] =  99;

    dhcp_option_t* opt = conv_option(dhcp->options, 0);
    // DHCP discover
    opt->code   = DHO_DHCP_MESSAGE_TYPE;
    opt->length = 1;
    opt->val[0] = DHCPDISCOVER;
    // DHCP client identifier
    opt = conv_option(dhcp->options, 3);
    opt->code   = DHO_DHCP_CLIENT_IDENTIFIER;
    opt->length = 7;
    opt->val[0] = HTYPE_ETHER;
    memcpy(&opt->val[1], &link_addr, ETH_ALEN);
    // DHCP Parameter Request Field
    opt = conv_option(dhcp->options, 12);
    opt->code   = DHO_DHCP_PARAMETER_REQUEST_LIST;
    opt->length = 3;
    opt->val[0] = DHO_ROUTERS;
    opt->val[1] = DHO_SUBNET_MASK;
    opt->val[2] = DHO_DOMAIN_NAME_SERVERS;
    // END
    opt = conv_option(dhcp->options, 17);
    opt->code   = DHO_END;
    opt->length = 0;

    ////////////////////////////////////////////////////////
    auto& socket = stack.udp().bind(DHCP_CLIENT_PORT);
    /// broadcast our DHCP plea as 0.0.0.0:67
    socket.bcast(IP4::ADDR_ANY, DHCP_SERVER_PORT, packet, PACKET_SIZE);

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
    const dhcp_packet_t* dhcp = (const dhcp_packet_t*) data;

    uint32_t xid = htonl(dhcp->xid);
    // silently ignore transactions not our own
    if (xid != this->xid) return;

    // check if the BOOTP message is a DHCP OFFER
    const dhcp_option_t* opt;
    const dhcp_option_t* server_id {nullptr};
    opt = get_option(dhcp->options, DHO_DHCP_MESSAGE_TYPE);

    if (opt->code == DHO_DHCP_MESSAGE_TYPE)
    {
      // verify that the type is indeed DHCPOFFER
      debug("Found DHCP message type %d  (DHCP Offer = %d)\n",
            opt->val[0], DHCPOFFER);

      // ignore when not a DHCP Offer
      if (opt->val[0] != DHCPOFFER) return;
    }
    // ignore message when DHCP message type is missing
    else return;

    // the offered IP address:
    this->ipaddr = dhcp->yiaddr;

    opt = get_option(dhcp->options, DHO_SUBNET_MASK);
    if (opt->code == DHO_SUBNET_MASK)
      memcpy(&this->netmask, opt->val, sizeof(IP4::addr));

    opt = get_option(dhcp->options, DHO_DHCP_LEASE_TIME);
    if (opt->code == DHO_DHCP_LEASE_TIME)
    {
      this->lease_time = (((uint32_t) opt->val[0]) << 24 |
        ((uint32_t) opt->val[1]) << 16 |
        ((uint32_t) opt->val[2]) <<  8 |
        ((uint32_t) opt->val[3]) <<  0);
    }

    // Preserve DHCP server address
    opt = get_option(dhcp->options, DHO_DHCP_SERVER_IDENTIFIER);
    if (opt->code == DHO_DHCP_SERVER_IDENTIFIER)
      {
        server_id = opt;
      }

    // now validate the offer, checking for minimum information
    opt = get_option(dhcp->options, DHO_ROUTERS);
    if (opt->code == DHO_ROUTERS)
    {
      memcpy(&this->router, opt->val, sizeof(IP4::addr));
    }
    // assume that the server we received the request from is the gateway
    else
    {
      if (server_id)
        memcpy(&this->router, server_id->val, sizeof(IP4::addr));
      // silently ignore when both ROUTER and SERVER_ID is missing
      else return;
    }

    opt = get_option(dhcp->options, DHO_DOMAIN_NAME_SERVERS);
    if (opt->code == DHO_DOMAIN_NAME_SERVERS)
    {
      memcpy(&this->dns_server, opt->val, sizeof(IP4::addr));
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

  void DHClient::request(UDPSocket& sock, const dhcp_option_t* server_id)
  {
    // form a response
    char packet[PACKET_SIZE];

    dhcp_packet_t* resp = (dhcp_packet_t*) packet;
    resp->op    = BOOTREQUEST;
    resp->htype = HTYPE_ETHER;
    resp->hlen  = ETH_ALEN;
    resp->hops  = 0;
    resp->xid   = htonl(this->xid);
    resp->secs  = 0;
    resp->flags = BOOTP_BROADCAST;

    resp->ciaddr = IP4::ADDR_ANY;
    resp->yiaddr = IP4::ADDR_ANY;
    resp->siaddr = IP4::ADDR_ANY;
    resp->giaddr = IP4::ADDR_ANY;

    MAC::Addr link_addr = stack.link_addr();

    // copy our hardware address to chaddr field
    memset(resp->chaddr, 0, dhcp_packet_t::CHADDR_LEN);
    memcpy(resp->chaddr, &link_addr, ETH_ALEN);
    // zero server, file and options
    memset(resp->sname, 0, dhcp_packet_t::SNAME_LEN + dhcp_packet_t::FILE_LEN);
    // magic DHCP bootp values
    resp->magic[0] =  99;
    resp->magic[1] = 130;
    resp->magic[2] =  83;
    resp->magic[3] =  99;

    dhcp_option_t* opt = conv_option(resp->options, 0);
    // DHCP Request
    opt->code   = DHO_DHCP_MESSAGE_TYPE;
    opt->length = 1;
    opt->val[0] = DHCPREQUEST;
    // DHCP client identifier
    opt = conv_option(resp->options, 3);
    opt->code   = DHO_DHCP_CLIENT_IDENTIFIER;
    opt->length = 7;
    opt->val[0] = HTYPE_ETHER;
    memcpy(&opt->val[1], &link_addr, ETH_ALEN);
    // DHCP server identifier
    opt = conv_option(resp->options, 12);
    opt->code   = DHO_DHCP_SERVER_IDENTIFIER;
    opt->length = 4;
    if (server_id) {
      memcpy(&opt->val[0], &server_id->val[0], sizeof(IP4::addr));
      MYINFO("Server IP set to %s", ((IP4::addr*)&opt->val[0])->to_string().c_str());
    } else {
      memcpy(&opt->val[0], &this->router, sizeof(IP4::addr));
      MYINFO("Server IP set to gateway (%s)", ((IP4::addr*)&opt->val[0])->to_string().c_str());
    }
    // DHCP Requested Address
    opt = conv_option(resp->options, 18);
    opt->code   = DHO_DHCP_REQUESTED_ADDRESS;
    opt->length = 4;
    memcpy(&opt->val[0], &this->ipaddr, sizeof(IP4::addr));
    // DHCP Parameter Request Field
    opt = conv_option(resp->options, 24);
    opt->code   = DHO_DHCP_PARAMETER_REQUEST_LIST;
    opt->length = 3;
    opt->val[0] = DHO_ROUTERS;
    opt->val[1] = DHO_SUBNET_MASK;
    opt->val[2] = DHO_DOMAIN_NAME_SERVERS;
    // END
    opt = conv_option(resp->options, 29);
    opt->code   = DHO_END;
    opt->length = 0;

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
    sock.bcast(IP4::ADDR_ANY, DHCP_SERVER_PORT, packet, PACKET_SIZE);
  }

  void DHClient::acknowledge(const char* data, size_t)
  {
    const dhcp_packet_t* dhcp = (const dhcp_packet_t*) data;

    uint32_t xid = htonl(dhcp->xid);
    // silently ignore transactions not our own
    if (xid != this->xid) return;

    // check if the BOOTP message is a DHCP OFFER
    const dhcp_option_t* opt;
    opt = get_option(dhcp->options, DHO_DHCP_MESSAGE_TYPE);

    if (opt->code == DHO_DHCP_MESSAGE_TYPE)
    {
      // verify that the type is indeed DHCPOFFER
      debug("\tFound DHCP message type %d  (DHCP Ack = %d)\n",
            opt->val[0], DHCPACK);
      // ignore when not a DHCP Offer
      if (opt->val[0] != DHCPACK) return;
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
