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

#define DEBUG
#include <info>
#define MYINFO(X,...) INFO("DHCPv4",X,##__VA_ARGS__)

#include <net/dhcp/dh4client.hpp>
#include <net/dhcp/dhcp4.hpp>
#include <kernel/os.hpp> // OS::cycles_since_boot()
#include <debug>

// BOOTP (rfc951) message types
#define BOOTREQUEST 1
#define BOOTREPLY   2

// Possible values for flags field
#define BOOTP_UNICAST   0x0000
#define BOOTP_BROADCAST 0x8000

// Possible values for hardware type (htype) field
#define HTYPE_ETHER     1  // Ethernet 10Mbps
#define HTYPE_IEEE802   6  // IEEE 802.2 Token Ring
#define HTYPE_FDDI      8  // FDDI

/* Magic cookie validating dhcp options field (and bootp vendor
   extensions field). */
#define DHCP_OPTIONS_COOKIE "\143\202\123\143"

// DHCP Option codes
#define DHO_PAD                  0
#define DHO_SUBNET_MASK          1
#define DHO_TIME_OFFSET          2
#define DHO_ROUTERS              3
#define DHO_TIME_SERVERS         4
#define DHO_NAME_SERVERS         5
#define DHO_DOMAIN_NAME_SERVERS  6
#define DHO_LOG_SERVERS          7
#define DHO_COOKIE_SERVERS       8
#define DHO_LPR_SERVERS          9
#define DHO_IMPRESS_SERVERS     10
#define DHO_RESOURCE_LOCATION_SERVERS   11
#define DHO_HOST_NAME           12
#define DHO_BOOT_SIZE           13
#define DHO_MERIT_DUMP          14
#define DHO_DOMAIN_NAME         15
#define DHO_SWAP_SERVER         16
#define DHO_ROOT_PATH           17
#define DHO_EXTENSIONS_PATH     18
#define DHO_IP_FORWARDING       19
#define DHO_NON_LOCAL_SOURCE_ROUTING 20
#define DHO_POLICY_FILTER            21
#define DHO_MAX_DGRAM_REASSEMBLY     22
#define DHO_DEFAULT_IP_TTL           23
#define DHO_PATH_MTU_AGING_TIMEOUT   24
#define DHO_PATH_MTU_PLATEAU_TABLE   25
#define DHO_INTERFACE_MTU            26
#define DHO_ALL_SUBNETS_LOCAL        27
#define DHO_BROADCAST_ADDRESS        28
#define DHO_PERFORM_MASK_DISCOVERY   29
#define DHO_MASK_SUPPLIER            30
#define DHO_ROUTER_DISCOVERY         31
#define DHO_ROUTER_SOLICITATION_ADDRESS 32
#define DHO_STATIC_ROUTES           33
#define DHO_TRAILER_ENCAPSULATION   34
#define DHO_ARP_CACHE_TIMEOUT       35
#define DHO_IEEE802_3_ENCAPSULATION 36
#define DHO_DEFAULT_TCP_TTL         37
#define DHO_TCP_KEEPALIVE_INTERVAL  38
#define DHO_TCP_KEEPALIVE_GARBAGE   39
#define DHO_NIS_DOMAIN              40
#define DHO_NIS_SERVERS             41
#define DHO_NTP_SERVERS             42
#define DHO_VENDOR_ENCAPSULATED_OPTIONS 43
#define DHO_NETBIOS_NAME_SERVERS    44
#define DHO_NETBIOS_DD_SERVER       45
#define DHO_NETBIOS_NODE_TYPE       46
#define DHO_NETBIOS_SCOPE           47
#define DHO_FONT_SERVERS            48
#define DHO_X_DISPLAY_MANAGER       49
#define DHO_DHCP_REQUESTED_ADDRESS  50
#define DHO_DHCP_LEASE_TIME         51
#define DHO_DHCP_OPTION_OVERLOAD    52
#define DHO_DHCP_MESSAGE_TYPE       53
#define DHO_DHCP_SERVER_IDENTIFIER  54
#define DHO_DHCP_PARAMETER_REQUEST_LIST 55
#define DHO_DHCP_MESSAGE            56
#define DHO_DHCP_MAX_MESSAGE_SIZE   57
#define DHO_DHCP_RENEWAL_TIME       58
#define DHO_DHCP_REBINDING_TIME     59
#define DHO_VENDOR_CLASS_IDENTIFIER 60
#define DHO_DHCP_CLIENT_IDENTIFIER  61
#define DHO_NWIP_DOMAIN_NAME        62
#define DHO_NWIP_SUBOPTIONS         63
#define DHO_USER_CLASS              77
#define DHO_FQDN                    81
#define DHO_DHCP_AGENT_OPTIONS      82
#define DHO_SUBNET_SELECTION       118  // RFC3011
/* The DHO_AUTHENTICATE option is not a standard yet, so I've
   allocated an option out of the "local" option space for it on a
   temporary basis.  Once an option code number is assigned, I will
   immediately and shamelessly break this, so don't count on it
   continuing to work. */
#define DHO_AUTHENTICATE           210
#define DHO_END                    255

// DHCP message types
#define DHCPDISCOVER 1
#define DHCPOFFER    2
#define DHCPREQUEST  3
#define DHCPDECLINE  4
#define DHCPACK      5
#define DHCPNAK      6
#define DHCPRELEASE  7
#define DHCPINFORM   8

// Relay Agent Information option subtypes
#define RAI_CIRCUIT_ID  1
#define RAI_REMOTE_ID   2
#define RAI_AGENT_ID    3

// FQDN suboptions
#define FQDN_NO_CLIENT_UPDATE 1
#define FQDN_SERVER_UPDATE    2
#define FQDN_ENCODED          3
#define FQDN_RCODE1           4
#define FQDN_RCODE2           5
#define FQDN_HOSTNAME         6
#define FQDN_DOMAINNAME       7
#define FQDN_FQDN             8
#define FQDN_SUBOPTION_COUNT  8

#define ETH_ALEN           6  // octets in one ethernet header
#define DHCP_DEST_PORT    67
#define DHCP_SOURCE_PORT  68

namespace net
{
  inline dhcp_option_t* conv_option(uint8_t* option)
  {
    return (dhcp_option_t*) option;
  }

  DHClient::DHClient(Stack& inet)
    : stack(inet), xid(0), console_spam(true), in_progress(false)
  {
    on_config([this] (bool timeout) {
        if (console_spam)
          {
            if (timeout)
              INFO("DHCPv4","Negotiation timed out");
            else
              INFO("DHCPv4","Config complete");
          }
      });
  }

  void DHClient::negotiate(uint32_t timeout_secs)
  {
    // Allow multiple calls to negotiate without restarting the process
    if (in_progress)
      return;

    in_progress = true;

    // set timeout handler
    using namespace std::chrono;
    this->timeout = Timers::oneshot(seconds(timeout_secs),
    [this] (uint32_t) {
      // reset session ID
      this->xid = 0;
      this->in_progress = false;

      // call on_config with timeout = true
      for(auto handler : this->config_handlers_)
        handler(true);
    });

    // create a random session ID
    this->xid = OS::cycles_since_boot() & 0xFFFFFFFF;
    if (console_spam)
      MYINFO("Negotiating IP-address (xid=%u)", xid);

    // create DHCP discover packet
    const size_t packetlen = sizeof(dhcp_packet_t);
    char packet[packetlen];

    dhcp_packet_t* dhcp = (dhcp_packet_t*) packet;
    dhcp->op    = BOOTREQUEST;
    dhcp->htype = HTYPE_ETHER;
    dhcp->hlen  = ETH_ALEN;
    dhcp->hops  = 0;
    dhcp->xid   = htonl(this->xid);
    dhcp->secs  = 0;
    dhcp->flags = htons(BOOTP_BROADCAST);
    dhcp->ciaddr = IP4::INADDR_ANY;
    dhcp->yiaddr = IP4::INADDR_ANY;
    dhcp->siaddr = IP4::INADDR_ANY;
    dhcp->giaddr = IP4::INADDR_ANY;

    Ethernet::addr link_addr = stack.link_addr();

    // copy our hardware address to chaddr field
    memset(dhcp->chaddr, 0, dhcp_packet_t::CHADDR_LEN);
    memcpy(dhcp->chaddr, &link_addr, ETH_ALEN);
    // zero server, file and options
    memset(dhcp->sname, 0, dhcp_packet_t::SNAME_LEN + dhcp_packet_t::FILE_LEN);

    dhcp->magic[0] =  99;
    dhcp->magic[1] = 130;
    dhcp->magic[2] =  83;
    dhcp->magic[3] =  99;

    dhcp_option_t* opt = conv_option(dhcp->options + 0);
    // DHCP discover
    opt->code   = DHO_DHCP_MESSAGE_TYPE;
    opt->length = 1;
    opt->val[0] = DHCPDISCOVER;
    // DHCP client identifier
    opt = conv_option(dhcp->options + 3);
    opt->code   = DHO_DHCP_CLIENT_IDENTIFIER;
    opt->length = 7;
    opt->val[0] = HTYPE_ETHER;
    memcpy(&opt->val[1], &link_addr, ETH_ALEN);
    // DHCP Parameter Request Field
    opt = conv_option(dhcp->options + 12);
    opt->code   = DHO_DHCP_PARAMETER_REQUEST_LIST;
    opt->length = 3;
    opt->val[0] = DHO_ROUTERS;
    opt->val[1] = DHO_SUBNET_MASK;
    opt->val[2] = DHO_DOMAIN_NAME_SERVERS;
    // END
    opt = conv_option(dhcp->options + 17);
    opt->code   = DHO_END;
    opt->length = 0;

    ////////////////////////////////////////////////////////
    auto& socket = stack.udp().bind(DHCP_SOURCE_PORT);
    /// broadcast our DHCP plea as 0.0.0.0:67
    socket.bcast(IP4::INADDR_ANY, DHCP_DEST_PORT, packet, packetlen);

    socket.on_read(
    [this, &socket] (IP4::addr, UDP::port_t port,
                     const char* data, size_t len)
    {
      if (port == DHCP_DEST_PORT)
      {
        // we have got a DHCP Offer
        debug("Received possible DHCP OFFER from %s:%d\n",
              addr.str().c_str(), DHCP_DEST_PORT);
        this->offer(socket, data, len);
      }
    });
  }

  const dhcp_option_t* get_option(const uint8_t* options, uint8_t code)
  {
    const dhcp_option_t* opt = (const dhcp_option_t*) options;
    while (opt->code != code && opt->code != DHO_END)
      {
        // go to next option
        opt = (const dhcp_option_t*) (((const uint8_t*) opt) + 2 + opt->length);
      }
    return opt;
  }

  void DHClient::offer(UDPSocket& sock, const char* data, size_t)
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
      debug("Found DHCP message type %d  (DHCP Offer = %d)\n",
            opt->val[0], DHCPOFFER);

      // ignore when not a DHCP Offer
      if (opt->val[0] != DHCPOFFER) return;
    }
    // ignore message when DHCP message type is missing
    else return;

    // the offered IP address:
    this->ipaddr = dhcp->yiaddr;
    if (console_spam)
      MYINFO("IP ADDRESS: \t%s", this->ipaddr.str().c_str());

    opt = get_option(dhcp->options, DHO_SUBNET_MASK);
    if (opt->code == DHO_SUBNET_MASK)
    {
      memcpy(&this->netmask, opt->val, sizeof(IP4::addr));
      if (console_spam)
        MYINFO("SUBNET MASK: \t%s", this->netmask.str().c_str());
    }

    opt = get_option(dhcp->options, DHO_DHCP_LEASE_TIME);
    if (opt->code == DHO_DHCP_LEASE_TIME)
    {
      memcpy(&this->lease_time, opt->val, sizeof(this->lease_time));
      if (console_spam)
        MYINFO("LEASE TIME: \t%u mins", this->lease_time / 60);
    }

    // now validate the offer, checking for minimum information
    opt = get_option(dhcp->options, DHO_ROUTERS);
    if (opt->code == DHO_ROUTERS)
    {
      memcpy(&this->router, opt->val, sizeof(IP4::addr));
      if (console_spam)
        MYINFO("GATEWAY: \t%s", this->router.str().c_str());
    }
    // assume that the server we received the request from is the gateway
    else
    {
      opt = get_option(dhcp->options, DHO_DHCP_SERVER_IDENTIFIER);
      if (opt->code == DHO_DHCP_SERVER_IDENTIFIER)
      {
        memcpy(&this->router, opt->val, sizeof(IP4::addr));
        if (console_spam)
          MYINFO("GATEWAY: \t%s", this->router.str().c_str());
      }
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
    if (console_spam)
      MYINFO("DNS SERVER: \t%s", this->dns_server.str().c_str());

    // we can accept the offer now by requesting the IP!
    this->request(sock);
  }

  void DHClient::request(UDPSocket& sock)
  {
    // form a response
    const size_t packetlen = sizeof(dhcp_packet_t);
    char packet[packetlen];

    dhcp_packet_t* resp = (dhcp_packet_t*) packet;
    resp->op    = BOOTREQUEST;
    resp->htype = HTYPE_ETHER;
    resp->hlen  = ETH_ALEN;
    resp->hops  = 0;
    resp->xid   = htonl(this->xid);
    resp->secs  = 0;
    resp->flags = htons(BOOTP_BROADCAST);

    resp->ciaddr = IP4::INADDR_ANY;
    resp->yiaddr = IP4::INADDR_ANY;
    resp->siaddr = IP4::INADDR_ANY;
    resp->giaddr = IP4::INADDR_ANY;

    Ethernet::addr link_addr = stack.link_addr();

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

    dhcp_option_t* opt = conv_option(resp->options + 0);
    // DHCP Request
    opt->code   = DHO_DHCP_MESSAGE_TYPE;
    opt->length = 1;
    opt->val[0] = DHCPREQUEST;
    // DHCP client identifier
    opt = conv_option(resp->options + 3);
    opt->code   = DHO_DHCP_CLIENT_IDENTIFIER;
    opt->length = 7;
    opt->val[0] = HTYPE_ETHER;
    memcpy(&opt->val[1], &link_addr, ETH_ALEN);
    // DHCP server identifier
    opt = conv_option(resp->options + 12);
    opt->code   = DHO_DHCP_SERVER_IDENTIFIER;
    opt->length = 4;
    memcpy(&opt->val[0], &this->router, sizeof(IP4::addr));
    // DHCP Requested Address
    opt = conv_option(resp->options + 18);
    opt->code   = DHO_DHCP_REQUESTED_ADDRESS;
    opt->length = 4;
    memcpy(&opt->val[0], &this->ipaddr, sizeof(IP4::addr));
    // DHCP Parameter Request Field
    opt = conv_option(resp->options + 24);
    opt->code   = DHO_DHCP_PARAMETER_REQUEST_LIST;
    opt->length = 3;
    opt->val[0] = DHO_ROUTERS;
    opt->val[1] = DHO_SUBNET_MASK;
    opt->val[2] = DHO_DOMAIN_NAME_SERVERS;
    // END
    opt = conv_option(resp->options + 29);
    opt->code   = DHO_END;
    opt->length = 0;

    // set our onRead function to point to a hopeful DHCP ACK!
    sock.on_read(
    [this] (IP4::addr, UDP::port_t port,
            const char* data, size_t len)
    {
      if (port == DHCP_DEST_PORT)
      {
        // we have hopefully got a DHCP Ack
        debug("\tReceived DHCP ACK from %s:%d\n",
              addr.str().c_str(), DHCP_DEST_PORT);
        this->acknowledge(data, len);
      }
    });

    // send our DHCP Request
    sock.bcast(IP4::INADDR_ANY, DHCP_DEST_PORT, packet, packetlen);
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

    if (console_spam)
      MYINFO("Server acknowledged our request!");

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
