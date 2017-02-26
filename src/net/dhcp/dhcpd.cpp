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

#include <net/dhcp/dhcpd.hpp>
#include <rtc>

#include <os>

using namespace net;
using namespace dhcp;

DHCPD::DHCPD(UDP& udp, IP4::addr pool_start, IP4::addr pool_end,
  uint32_t lease, uint32_t max_lease, uint8_t pending)
: stack_{udp.stack()},
  socket_{udp.bind(DHCP_SERVER_PORT)},
  pool_start_{pool_start},
  pool_end_{pool_end},
  server_id_{stack_.ip_addr()},
  netmask_{stack_.netmask()},
  router_{stack_.gateway()},
  dns_{stack_.gateway()},
  lease_{lease}, max_lease_{max_lease}, pending_{pending}
{
  if (not valid_pool(pool_start, pool_end))
    throw DHCP_exception{"Invalid pool"};

  init_pool();
  listen();
}

bool DHCPD::record_exists(const Record::byte_seq& client_id) const noexcept {
  for (auto& rec : records_)
    if (rec.client_id() == client_id)
      return true;

  return false;
}

int DHCPD::get_record_idx(const Record::byte_seq& client_id) const noexcept {
  for (size_t i = 0; i < records_.size(); i++)
    if (records_.at(i).client_id() == client_id)
      return i;

  return -1;
}

int DHCPD::get_record_idx_from_ip(IP4::addr ip) const noexcept {
  for (size_t i = 0; i < records_.size(); i++)
    if (records_.at(i).ip() == ip)
      return i;

  return -1;
}

bool DHCPD::valid_pool(IP4::addr start, IP4::addr end) const {
  if (start > end or start == end or inc_addr(start) == end)
      return false;

  // Check if start and end are on the same subnet
  if (network_address(start) != network_address(end)
    or network_address(start) != network_address(server_id_)
    or network_address(end) != network_address(server_id_))
    return false;

  // IP address part 0 is the rightmost uint8_t
  // IP address part 3 is the leftmost uint8_t
  const uint8_t start3 = start.part(3);
  const uint8_t start2 = start.part(2);
  const uint8_t start0 = start.part(0);
  const uint8_t end3 = end.part(3);
  const uint8_t end2 = end.part(2);
  const uint8_t end0 = end.part(0);

  // 10.x.x.x addresses not valid
  // 172.16.0.0 - 172.31.255.255 not valid
  // 192.168.x.x addresses not valid

  // TODO: 10.x.x.x addresses should be invalid
  if (/* start3 == 10 or */(start3 == 192 and start2 == 168) or start0 < 1 or start0 > 254)
    return false;

  // TODO: 10.x.x.x addresses should be invalid
  if (/* end3 == 10 or */(end3 == 192 and end2 == 168) or end0 < 1 or end0 > 254)
    return false;

  if ( (start3 == 172 and (start2 > 15 or start2 < 32)) or
    (end3 == 172 and (end2 > 15 or end2 < 32)) )
    return false;

  return true;
}

void DHCPD::init_pool() {
  IP4::addr start = pool_start_;
  while (start < pool_end_ and network_address(start) == network_address(server_id_)) {
    pool_.emplace(std::make_pair(start, Status::AVAILABLE));
    start = inc_addr(start);
  }
  // Incl. pool_end_ IP:
  pool_.emplace(std::make_pair(pool_end_, Status::AVAILABLE));
}

void DHCPD::update_pool(IP4::addr ip, Status new_status) {
  auto it = pool_.find(ip);
  if (it not_eq pool_.end())
    it->second = new_status;
}

void DHCPD::listen() {
  socket_.on_read([&] (IP4::addr, UDP::port_t port,
    const char* data, size_t) {

    if (port == DHCP_CLIENT_PORT) {
      //printf("Received data from DHCP client\n");

      // Message
      const dhcp_packet_t* msg = (dhcp_packet_t*) data;

      // Options
      const dhcp_option_t* opt = (dhcp_option_t*) msg->options;

      // Find out what kind of DHCP message this is
      resolve(msg, opt);
    }
  });
}

void DHCPD::resolve(const dhcp_packet_t* msg, const dhcp_option_t* opts) {
  int ridx;

  // TODO: If we have a record on this client already, reuse IP - but only relevant when get a REQUEST? (handle_request())

  switch (opts->val[0]) {
    case DHCPDISCOVER:
      //printf("\nDISCOVER\n");
      offer(msg, opts);
      break;
    case DHCPREQUEST:
      //printf("\nREQUEST\n");
      handle_request(msg, opts);
      break;
    case DHCPDECLINE:
      //printf("\nDECLINE\n");

      // A client has discovered that the suggested network address is already in use.
      // The server must mark the network address as not available and should notify the network admin
      // of a possible configuration problem - TODO

      ridx = get_record_idx(get_client_id(msg->chaddr, opts));

      // TODO
      // Erase record if exists because client hasn't got a valid IP
      if (ridx not_eq -1)
        records_.erase(records_.begin() + ridx);
      update_pool(get_requested_ip_in_opts(opts), Status::IN_USE);    // In use - possible config problem
      break;
    case DHCPRELEASE:
      //printf("\nRELEASE\n");

      // Mark the network address as not allocated anymore
      // Should retain a record of the client's initialization parameters for possible reuse
      // in response to subsequent requests from the client

      ridx = get_record_idx_from_ip(msg->ciaddr);

      if (ridx not_eq -1)
        records_.erase(records_.begin() + ridx);
      update_pool(msg->ciaddr, Status::AVAILABLE);
      // TODO
      // Or: Keep config parameters (record) and set status to RELEASED f.ex. instead (but need to handle this -
      // when a new discover (DHCPDISCOVER after DHCPRELEASE) is received - look through records and find record
      // with client id):
      /*if (ridx not_eq -1)
        records_.at(ridx).set_status(Status::RELEASED);
      update_pool(msg->ciaddr, Status::RELEASED);
      */
      break;
    case DHCPINFORM:            // ? Not specified in RFC 1533
      //printf("\nINFORM\n");

      // Send a DHCPACK directly to the address given in the ciaddr field of the DHCPINFORM message.
      // Do not send a lease expiration time to the client and do not fill in yiaddr
      inform_ack(msg, opts);
      break;
  }
}

void DHCPD::handle_request(const dhcp_packet_t* msg, const dhcp_option_t* opts) {

  // Client could be:
  // 1. Requesting offered parameters from one server and implicitly declining offers from all others
  // 2. Confirming correctness of previously allocated address after, e.g., system reboot
  // 3. Extending the lease on a particular network address
  // /
  // 1. Client is responding to a DHCPOFFER message from a server
  // 2. Client is verifying a previously allocated IP address
  // 3. Client is extending the lease on a network address

  // RFC 2131 PAGE 31 (pdf)

  // CHECKING SERVER IDENTIFIER
  const dhcp_option_t* opt = get_option(opts, DHO_DHCP_SERVER_IDENTIFIER);
  // Server identifier from client:
  if (opt->code == DHO_DHCP_SERVER_IDENTIFIER) {
    // Then this is a response to a DHCPOFFER message

    //printf("REQUEST from client is a response to a DHCPOFFER and has a SERVER IDENTIFIER\n");

    IP4::addr sid{opt->val[0], opt->val[1], opt->val[2], opt->val[3]};

    if (sid not_eq server_id()) {
      //printf("The client has not chosen this server\n");

      int ridx = get_record_idx(get_client_id(msg->chaddr, opts));
      if (ridx != -1) {
        // Free up the IP address - the client has declined our offer
        update_pool(records_.at(ridx).ip(), Status::AVAILABLE);
        records_.erase(records_.begin() + ridx);
      }
      return;
    }

    // SERVER IDENTIFIER == THIS SERVER'S ID
    //printf("Server identifier matches this server\n");

    int ridx = get_record_idx(get_client_id(msg->chaddr, opts));

    if (ridx not_eq -1) {   // if record exists
      //printf("A record for this client already exists\n");

      auto& record = records_.at(ridx);
      // if ciaddr is zero and requested IP address is filled in with the yiaddr value from the chosen DHCPOFFER
      // Client state: SELECTING
      if (msg->ciaddr == IP4::addr{0} and get_requested_ip_in_opts(opts) == record.ip()) {
        // Then the DHCPREQUEST from the client is a selecting request
        // Note: Servers may not receive a specific DHCPREQUEST from which they can decide whether or not the
        // client has accepted the offer. ... Servers are free to reuse offered network addresses in response to
        // subsequent requests. Servers should not reuse offered addresses and may use an implementation-specific
        // timeout mechanism to decide when to reuse an offered address.

        //printf("This is a selecting request from the client - will construct DHCPACK\n");

        // RECORD
        record.set_status(Status::IN_USE);

        // POOL
        update_pool(record.ip(), Status::IN_USE);

        // SEND DHCPACK
        request_ack(msg, opts);
        return;
      }

      //printf("CIADDR IS NOT ZERO OR THE RECORD'S IP DOES NOT MATCH THE REQUESTED IP IN OPTS FROM CLIENT - the client probably has already gotten an IP from the server - dhcpnak\n");
      // ELSE ciaddr is not zero or record's ip is not the same as the requested ip in the request
      // TODO: nak() or silent return?
      nak(msg);
      return;
    }

    // Else a record with this client id doesn't exist
    // TODO: Remain silent or send a DHCPNAK?
    //printf("RECORD FOR THIS CLIENT does not exist - sending DHCPNAK\n");
    nak(msg);
    return;
  }

  // Else server identifier is not filled in and this is not a response to a DHCPOFFER message
  // This is a request to verify or extend an existing lease

  //printf("Server identifier is NOT filled in and this is a request to verify or extend an existing lease\n");

  verify_or_extend_lease(msg, opts);

  // 4.3.2

  // Selecting
  // Server identifier filled in, ciaddr is zero, requested IP address is filled in with the yiaddr
  // value from the chosen DHCPOFFER
  // Note: Servers may not receive a specific DHCPREQUEST from which they can decide whether or not the
  // client has accepted the offer. ... Servers are free to reuse offered network addresses in response to
  // subsequent requests. Servers should not reuse offered addresses and may use an implementation-specific
  // timeout mechanism to decide when to reuse an offered address.

  // Init-reboot
  // If server identifier is not filled in and requested IP address option is filled in with
  // client's notion of its previously assigned address.
  // ciaddr must be zero.
  // The client is seeking to verify a previously allocated, cached configuration.
  // Server should send a DHCPNAK message to the client if the requested IP address is incorrect or is
  // on the wrong network.
  // Determining whether the client is on the correct network: Examine the contents of giaddr + the
  // requested IP address option + a database lookup. If the server detects that the client is on the wrong
  // net (i.e., the result of applying the local subnet mask or remote subnet mask (if giaddr is not zero) to
  // requested IP address option value doesn't match reality), then the server should send a DHCPNAK message
  // to the client.
  // If the network is correct: Server should check if the client's notion of its IP address is correct. If not,
  // the server should send a DHCPNAK message to the client.
  // If the server has no record of the client, it must remain silent and may output a warning to the network admin.
  // If giaddr is 0x0 in the DHCPREQUEST message, the client is on the same subnet as the server. The server must
  // broadcast the DHCPNAK message to the 0xffffffff broadcast address because teh client may not have a correct
  // network address or subnet mask, and the client may not be answering ARP requests.
  // If giaddr is set in the DHCPREQUEST, the client is on a different subnet. The server must set the broadcast bit
  // in the DHCPNAK - the client may not have a correct network address or subnet mask, and the client may not be
  // answering ARP requests.

  // Renewing
  // If server identifier and requested IP address option is not filled in
  // and ciaddr is filled in with client's IP address
  // Then the client is completely configured and is trying to extend its lease.
  // This message will be unicast. Because giaddr is therefore not filled in, the server
  // will trust the value in ciaddr and use it when replying to the client.

  // Rebinding
  // If server identifier and requested IP address option is not filled in
  // and ciaddr is filled in with client's IP address
  // Then the client is completely configured and is trying to extend its lease.
  // This message must be broadcast to the 0xffffffff IP broadcast address.
  // Server should check ciaddr for correctness before replying to the DHCPREQUEST.
  // A server may extend a client's lease only if it has local administrative authority to do so.

  // See Table 3 and 4.3.2 RFC 2131
}

void DHCPD::verify_or_extend_lease(const dhcp_packet_t* msg, const dhcp_option_t* opts) {
  int ridx = get_record_idx(get_client_id(msg->chaddr, opts));

  if (ridx == -1) {
    // If the server has no record of this client, then it MUST remain silent and MAY
    // output a warning to the network administrator.
    return;
  }

  if (ridx not_eq -1 and msg->ciaddr == IP4::addr{0}) {

    // Then the client is seeking to verify a previously allocated, cached configuration
    // Client state: INIT-REBOOT

    //printf("Client is in init-reboot state\n");

    // 1. Check if on correct network
    // Server should send a DHCPNAK if the client is on the wrong network
    // If the server detects that the client is on the wrong net (i.e., the result of applying
    // the local subnet mask or remote subnet mask (if giaddr is not zero) to requested IP address value doesn't
    // match reality), then the server should send a DHCPNAK message to the client.
    if (not on_correct_network(msg->giaddr, opts)) {
      //printf("Client is on the wrong network\n");

      // TODO: Update pool? Erase record?
      update_pool(records_.at(ridx).ip(), Status::AVAILABLE);
      records_.erase(records_.begin() + ridx);
      nak(msg);
      return;
    }

    // 2. If is on correct network: Check if the client's notion of IP is correct
    if (get_requested_ip_in_opts(opts) not_eq records_.at(ridx).ip()) {
      // If not correct IP: DHCPNAK + update pool and remove record
      update_pool(records_.at(ridx).ip(), Status::AVAILABLE);
      records_.erase(records_.begin() + ridx);
      nak(msg);
      return;
    }

    // TODO
    // Else everything is OK and we want to verify the client's configuration
    // Update pool and record too
    auto& record = records_.at(ridx);
    record.set_status(Status::IN_USE);
    record.set_lease_start(RTC::now());
    update_pool(record.ip(), Status::IN_USE);

    // Send DHCPACK?
    request_ack(msg, opts);
    return;
  }

  if (get_requested_ip_in_opts(opts) == IP4::addr{0} and msg->ciaddr not_eq IP4::addr{0}) {

    // Client is in RENEWING state
    // The client is then completely configured and is trying to extend its lease.
    // This message will be unicast, so no relay agents will be involved in its transmission.
    // Because giaddr is therefore not filled in, the server will trust the value in ciaddr and
    // use it when replying to the client.
    // A client MAY choose to renew or extend its lease prior to T1. The server may choose not to extend
    // the lease (as a policy decision by the network administrator), but should return a DHCPACK
    // message regardless.

    // Same if the client is in REBINDING state so check the giaddr (gateway IP address) to find out if message
    // has been unicast to the server or not? If zero: Unicast. If non-zero: Not unicast ?
    // REBINDING: The client is completely configured and is trying to extend its lease. This message MUST
    // be broadcast to the 0xffffffff IP broadcast address. The server should check ciaddr for correctness
    // before replying to the DHCPREQUEST

    // TABLE 4 PAGE 33 IN RFC 2131 - Fields in client message from client in different state

    // Record exists
    // Extend the lease or not - should send a DHCPACK regardless
    // Update record and pool?
    auto& record = records_.at(ridx);
    record.set_status(Status::IN_USE);
    record.set_lease_start(RTC::now());
    update_pool(record.ip(), Status::IN_USE);

    request_ack(msg, opts);
  }
}

void DHCPD::offer(const dhcp_packet_t* msg, const dhcp_option_t* opts) {

  // May occur that the client doesn't get our offers and then we don't want to
  // exhaust our pool
  int ridx = get_record_idx(get_client_id(msg->chaddr, opts));
  if (ridx not_eq -1) {
    //printf("Getting DHCPDISCOVER from a client that already exists/has a record\n");
    //printf("Status of the lease: %d\n", records_.at(i).status());
    return;
  }

  // 4.3.1 and Table 3 in RFC 2131
  // Choose a network address
  // Choose an expiration time for the lease
  // +

  const size_t packetlen = sizeof(dhcp_packet_t);
  char packet[packetlen];

  // If we want to offer this client an IP address:
  // RECORD
  Record record;

  // Keep client identifier (index into database)
  record.set_client_id(get_client_id(msg->chaddr, opts));

  // OFFER
  dhcp_packet_t* offer = (dhcp_packet_t*) packet;

  offer->op = BOOTREPLY;
  offer->htype = 1;   // From "Assigned Numbers" RFC. 1 = 10mb ethernet
  offer->hlen = 6;    // Hardware address length in octets. 6 for 10mb ethernet
  offer->hops = 0;
  offer->xid = msg->xid;
  offer->secs = 0;
  offer->ciaddr = IP4::addr{0};

  bool ok = false;

  // yiaddr
  // IP address offered to client - Available from the pool
  for (auto& entry : pool_) {
    if (entry.second == Status::AVAILABLE) {
      entry.second = Status::OFFERED;
      offer->yiaddr = entry.first;
      ok = true;

      // Servers need not reserve the offered network address, although the protocol will work more efficiently if the server
      // avoids allocating the offered network address to another client.
      // When allocating a new address, servers should check that the offered network address is not already in use; e.g.
      // the server may probe the offered address with an ICMP Echo Request. Servers SHOULD be implemented so that
      // network admins MAY choose to disable probes of newly allocated addresses.

      // RECORD
      record.set_ip(entry.first);
      record.set_status(Status::OFFERED);
      record.set_lease_start(RTC::now());
      record.set_lease_duration(lease_);

      break;
    }
  }

  if (not ok) {
    //printf("No IP is available - sending DHCPNAK and at the same time cleaning up OFFERED IPs that have not changed status\n");

    // Can then loop through all the records and erasing the ones that have status OFFERED
    for (size_t i = 0; i < records_.size(); i++)
      if (records_.at(i).status() == Status::OFFERED)
        records_.erase(records_.begin() + i);

    // If get here: No available IP and send:
    nak(msg);   // record has not been added to the records_ vector so OK
    return;
  }

  offer->siaddr = IP4::addr{0};   // IP address of next bootstrap server
  offer->flags = msg->flags;
  offer->giaddr = msg->giaddr;
  std::memcpy(offer->chaddr, msg->chaddr, dhcp_packet_t::CHADDR_LEN); // uint8_t array - size 16, client hardware address
  std::memcpy(offer->sname, msg->sname, dhcp_packet_t::SNAME_LEN);  // Server host name or options - MAY
  std::memcpy(offer->file, msg->file, dhcp_packet_t::FILE_LEN);     // Client boot file name or options - MAY
  offer->magic[0] =  99;
  offer->magic[1] = 130;
  offer->magic[2] =  83;
  offer->magic[3] =  99;

  // OPTIONS
  dhcp_option_t* offer_opts = (dhcp_option_t*) (offer->options + 0);

  // In DHCPOFFER Options:
  // Requested IP address                                         MUST NOT
  // ! IP address lease time                                      MUST
  // Use file/sname fields                                        MAY
  // DHCP message type                                            DHCPOFFER
  // Parameter request list (DHO_DHCP_PARAMETER_REQUEST_LIST)     MUST NOT
  // ! Message (DHO_DHCP_MESSAGE)                                 SHOULD
  // Client identifier                                            MUST NOT - note RFC 6842
  // Vendor class identifier                                      MAY
  // ! Server identifier                                          MUST
  // Maximum message size                                         MUST NOT
  // All others                                                   MAY

  // MESSAGE_TYPE
  offer_opts->code = DHO_DHCP_MESSAGE_TYPE;
  offer_opts->length = 1;
  offer_opts->val[0] = DHCPOFFER;

  // SERVER_IDENTIFIER (the server's network address(es))
  offer_opts = (dhcp_option_t*) (offer->options + 3);         // 3 bytes filled in prior
  add_server_id(offer_opts);

  // SUBNET_MASK
  offer_opts = (dhcp_option_t*) (offer->options + 9);         // 9 bytes filled in prior
  offer_opts->code = DHO_SUBNET_MASK;
  offer_opts->length = 4;
  offer_opts->val[0] = netmask_.part(3);
  offer_opts->val[1] = netmask_.part(2);
  offer_opts->val[2] = netmask_.part(1);
  offer_opts->val[3] = netmask_.part(0);

  // ROUTERS
  offer_opts = (dhcp_option_t*) (offer->options + 15);            // 15 bytes filled in prior
  offer_opts->code = DHO_ROUTERS;
  offer_opts->length = 4;
  offer_opts->val[0] = router_.part(3);
  offer_opts->val[1] = router_.part(2);
  offer_opts->val[2] = router_.part(1);
  offer_opts->val[3] = router_.part(0);

  // LEASE_TIME
  // Time in units of seconds (32-bit unsigned integer / 4 uint8_t)
  offer_opts = (dhcp_option_t*) (offer->options + 21);            // 21 bytes filled in prior
  offer_opts->code = DHO_DHCP_LEASE_TIME;
  offer_opts->length = 4;
  offer_opts->val[0] = (lease_ & 0xff000000) >> 24;
  offer_opts->val[1] = (lease_ & 0x00ff0000) >> 16;
  offer_opts->val[2] = (lease_ & 0x0000ff00) >> 8;
  offer_opts->val[3] = (lease_ & 0x000000ff);

  // MESSAGE (SHOULD)
  // No error message (only in DHCPNAK)
  offer_opts = (dhcp_option_t*) (offer->options + 27);            // 27 bytes filled in prior
  offer_opts->code = DHO_DHCP_MESSAGE;
  offer_opts->length = 1;                                 // No error message (only in DHCPNAK)
  offer_opts->val[0] = 0;

  // Testing on ubuntu - Adding options:
  // 1 TIME_OFFSET
  offer_opts = (dhcp_option_t*) (offer->options + 30);
  offer_opts->code = DHO_TIME_OFFSET;
  offer_opts->length = 4;
  offer_opts->val[0] = 0;
  offer_opts->val[1] = 0;
  offer_opts->val[2] = 0;
  offer_opts->val[3] = 0;

  // 2 DOMAIN_NAME_SERVERS
  offer_opts = (dhcp_option_t*) (offer->options + 36);
  offer_opts->code = DHO_DOMAIN_NAME_SERVERS;
  offer_opts->length = 4;
  offer_opts->val[0] = dns_.part(3);
  offer_opts->val[1] = dns_.part(2);
  offer_opts->val[2] = dns_.part(1);
  offer_opts->val[3] = dns_.part(0);

  // 3 HOST_NAME - responding with client's host name (but not specified in rfc?)
  offer_opts = (dhcp_option_t*) (offer->options + 42);
  offer_opts->code = DHO_HOST_NAME;
  auto host_name_length = 0;
  const dhcp_option_t* host_name = get_option(opts, DHO_HOST_NAME);
  if (host_name->code == DHO_HOST_NAME) {
    host_name_length = host_name->length;
    offer_opts->length = host_name_length;
    for (int i = 0; i < host_name->length; i++)
      offer_opts->val[i] = host_name->val[i];
  } else {
    host_name_length = 1;
    offer_opts->length = 1;
    offer_opts->val[0] = 0;
  }

  // 4 DOMAIN_NAME - blank for now
  offer_opts = (dhcp_option_t*) (offer->options + 44 + host_name_length);
  offer_opts->code = DHO_DOMAIN_NAME;
  offer_opts->length = 1;
  offer_opts->val[0] = 0;

  // 5 BROADCAST_ADDRESS -
  offer_opts = (dhcp_option_t*) (offer->options + 47 + host_name_length);
  offer_opts->code = DHO_BROADCAST_ADDRESS;
  offer_opts->length = 4;
  auto broadcast = broadcast_address();
  offer_opts->val[0] = broadcast.part(3);
  offer_opts->val[1] = broadcast.part(2);
  offer_opts->val[2] = broadcast.part(1);
  offer_opts->val[3] = broadcast.part(0);

  // END
  offer_opts = (dhcp_option_t*) (offer->options + 53 + host_name_length);    // 30 bytes filled in prior
  offer_opts->code   = DHO_END;
  offer_opts->length = 0;

  // The client should include the maximum DHCP message size option to let the server know how large the server may
  // make its DHCP messages. The parameters returned to a client may still exceed the space allocated to options in a
  // DHCP message. In this case, two additional options flags (which must appear in the options field of the message)
  // indicate that the file and sname fields are to be used for options.

  // The client may suggest values for the network address and lease time in the DHCPDISCOVER message. The client may
  // include the requested IP address option to suggest that a particular IP address be assigned, and may include the
  // IP address lease time option to suggest the lease time it would like. Other options representing hints at configuration
  // parameters are allowed in a DHCPDISCOVER or DHCPREQUEST message. However, additional options may be ignored by
  // servers, and multiple servers may, therefore, not return identical values for some options.
  // The requested IP address options is to be filled in only in a DHCPREQUEST message when the client is verifying network
  // parameters obtained previously. The client fills in the ciaddr field only when correctly configured with an IP address
  // in BOUND, RENEWING or REBINDING state.

  // THE DHCPDISCOVER MESSAGE (msg) MAY include options that suggest values for the network address and lease duration
  // TODO
  // If the client included a list of requested parameters in a DHCPDISCOVER message, it MUST
  // include that list in all subsequent messages.
  // Any configuration parameters in the DHCPACK message SHOULD NOT conflict with those in the earlier
  // DHCPOFFER message to which the client is responding.
  // -> When giving the client what it has requested, store these parameters in the Record created
  //Alt.
  //Option op;
  //op.set_dns(dns_);
  //op.set_netmask(netmask_);
  //record.options_.push_back(op);
  // RECORD: Add configuration options
  /*const dhcp_option_t* opt = get_option(opts, DHO_DHCP_PARAMETER_REQUEST_LIST);
  if (opt->code == DHO_DHCP_PARAMETER_REQUEST_LIST) {
    printf("PARAMETER REQUEST LIST:\n");
    for (int i = 0; i < opt->length; i++) {
      printf("Option: %u\n", opt->val[i]);
      // Return the requested options to the client, f.ex. dns
      // Possible to avoid switch case?
      //Alt.
      //Option op;
      //op.set_dns(dns_);
      //op.set_netmask(netmask_);
      //record.options_.push_back(op);
    }
    printf("\n");
  }*/

  // RECORD
  add_record(record);

  //printf("Sending DHCPOFFER\n");

  // If the giaddr field in a DHCP message from a client is non-zero, the server
  // sends any return messages to the DHCP server port on the BOOTP relay agent
  // whose address appears in giaddr
  // DHCP server port is port 67
  // DHCP client port is port 68
  if (msg->giaddr != IP4::addr{0}) {
    socket_.sendto(msg->giaddr, DHCP_SERVER_PORT, packet, packetlen);
    return;
  }

  // msg->giaddr is zero
  // If the giaddr field is zero and the ciaddr field is non-zero, then the server
  // unicasts DHCPOFFER and DHCPACK messages to the address in ciaddr
  if (msg->ciaddr != IP4::addr{0}) {
    socket_.sendto(msg->ciaddr, DHCP_CLIENT_PORT, packet, packetlen);
    return;
  }

  // msg->giaddr and msg->ciaddr is zero
  // If giaddr is zero and ciaddr is zero, and the broadcast bit (leftmost bit in
  // flags field) is set, then the server broadcasts DHCPOFFER and DHCPACK messsages
  // to 0xffffffff
  std::bitset<8> bits(msg->flags);
  if (bits[7] == 1) {
    socket_.bcast(server_id_, DHCP_CLIENT_PORT, packet, packetlen);
    return;
  }

  // the broadcast bit is not set
  // If the broadcast bit is not set and giaddr is zero and ciaddr is zero, then the
  // server unicasts DHCPOFFER and DHCPACK messages to the client's hardware address
  // and yiaddr address
  socket_.sendto(msg->yiaddr, DHCP_CLIENT_PORT, packet, packetlen);
  // TODO: Send the message to the client's hardware address:
  // socket_.sendto(, DHCP_CLIENT_PORT, packet, packetlen);
}

void DHCPD::inform_ack(const dhcp_packet_t* msg, const dhcp_option_t* opts) {
  // If the client included a list of requested parameters in a DHCPDISCOVER message, it MUST
  // include that list in all subsequent messages.
  // Any configuration parameters in the DHCPACK message SHOULD NOT conflict with those in the earlier
  // DHCPOFFER message to which the client is responding.

  // Send a DHCPACK directly to the address given in the ciaddr field of the DHCPINFORM message.
  // Do not send a lease expiration time to the client and do not fill in yiaddr

  // NB: Option IP address lease time MUST NOT

  // The client has obtained a network address through som other means (e.g., manual configuration)
  // and may use a DHCPINFORM request message to obtain other local configuration parameters.
  // Servers receiving a DHCPINFORM message construct a DHCPACK message with any local configuration
  // parameters appropriate for the client without:
  // - allocating a new address
  // - checking for an existing binding
  // - filling in yiaddr
  // - including lease time parameters
  // The servers should unicast the DHCPACK reply to the address given in the ciaddr field of the
  // DHCPINFORM message.

  // The server should check the network address in the DHCPINFORM message for consistency, but
  // must not check for an existing lease. The server forms a DHCPACK message containing the
  // configuration parameters for the requesting client and sends the DHCPACK message directly to
  // the client.

  const size_t packetlen = sizeof(dhcp_packet_t);
  char packet[packetlen];

  // ACK
  dhcp_packet_t* ack = (dhcp_packet_t*) packet;

  ack->op = BOOTREPLY;
  ack->htype = 1; // From "Assigned Numbers" RFC. 1 = 10mb ethernet
  ack->hlen = 6;  // Hardware address length in octets. 6 for 10mb ethernet
  ack->hops = 0;
  ack->xid = msg->xid;
  ack->secs = 0;
  ack->ciaddr = msg->ciaddr;      // or 0
  ack->siaddr = IP4::addr{0};     // TODO IP address of next bootstrap server
  ack->flags = msg->flags;
  ack->giaddr = msg->giaddr;
  std::memcpy(ack->chaddr, msg->chaddr, dhcp_packet_t::CHADDR_LEN);
  std::memcpy(ack->sname, msg->sname, dhcp_packet_t::SNAME_LEN);  // Server host name or options - MAY
  std::memcpy(ack->file, msg->file, dhcp_packet_t::FILE_LEN);     // Client boot file name or options - MAY
  ack->magic[0] =  99;
  ack->magic[1] = 130;
  ack->magic[2] =  83;
  ack->magic[3] =  99;

  // OPTIONS
  dhcp_option_t* ack_opts = (dhcp_option_t*) (ack->options + 0);

  // MESSAGE_TYPE
  ack_opts->code = DHO_DHCP_MESSAGE_TYPE;
  ack_opts->length = 1;
  ack_opts->val[0] = DHCPACK;

  // And more options

  // TODO Options requested by the client:
  /*const dhcp_option_t* opt = get_option(opts, DHO_DHCP_PARAMETER_REQUEST_LIST);
  if (opt->code == DHO_DHCP_PARAMETER_REQUEST_LIST) {
    printf("PARAMETER REQUEST LIST IN DHCPINFORM MESSAGE:\n");
    for (int i = 0; i < opt->length; i++) {
      printf("Option: %u\n", opt->val[i]);
      // Return the requested options to the client, f.ex. dns
      // Possible to avoid switch case?
    }
    printf("\n");
  }*/

  //printf("Sending an inform ack to ciaddr: %s\n", msg->ciaddr.to_string().c_str());

  // Send the DHCPACK
  // Unicast to DHCPINFORM message's ciaddr field
  socket_.sendto(msg->ciaddr, DHCP_CLIENT_PORT, packet, packetlen);
}

void DHCPD::request_ack(const dhcp_packet_t* msg, const dhcp_option_t* opts) {

  // If the client included a list of requested parameters in a DHCPDISCOVER message, it MUST
  // include that list in all subsequent messages.
  // Any configuration parameters in the DHCPACK message SHOULD NOT conflict with those in the earlier
  // DHCPOFFER message to which the client is responding.

  // NB: Option IP address lease time MUST

  // DHCPACK: Server to client with configuration parameters, including committed network address

  // Table 3 RFC 2131 DHCPACK
  // op           BOOTREPLY
  // htype        From "Assigned Numbers" RFC
  // hlen         Hardware address length in octets
  // hops         0
  // xid          xid from client DHCPREQUEST
  // etc.

  // The client broadcasts a DHCPREQUEST message that MUST include the server identifier option
  // to indicate which server it has selected, and that MAY include other options specifying desired configuration values.
  // The requested IP address option MUST be set to the value of yiaddr in the DHCPOFFER message from the server.

  const size_t packetlen = sizeof(dhcp_packet_t);
  char packet[packetlen];

  // ACK
  dhcp_packet_t* ack = (dhcp_packet_t*) packet;

  ack->op = BOOTREPLY;
  ack->htype = 1; // From "Assigned Numbers" RFC. 1 = 10mb ethernet
  ack->hlen = 6;  // Hardware address length in octets. 6 for 10mb ethernet
  ack->hops = 0;
  ack->xid = msg->xid;
  ack->secs = 0;
  ack->ciaddr = msg->ciaddr;      // or 0

  int ridx = get_record_idx(get_client_id(msg->chaddr, opts));
  ack->yiaddr = (ridx not_eq -1) ? records_.at(ridx).ip() : IP4::addr{0}; // TODO: else what?     // IP address assigned to client

  ack->siaddr = IP4::addr{0};     // TODO IP address of next bootstrap server
  ack->flags = msg->flags;
  ack->giaddr = msg->giaddr;
  std::memcpy(ack->chaddr, msg->chaddr, dhcp_packet_t::CHADDR_LEN);
  std::memcpy(ack->sname, msg->sname, dhcp_packet_t::SNAME_LEN);  // Server host name or options - MAY
  std::memcpy(ack->file, msg->file, dhcp_packet_t::FILE_LEN);     // Client boot file name or options - MAY
  ack->magic[0] =  99;
  ack->magic[1] = 130;
  ack->magic[2] =  83;
  ack->magic[3] =  99;

  // OPTIONS
  dhcp_option_t* ack_opts = (dhcp_option_t*) (ack->options + 0);

  // MESSAGE_TYPE
  ack_opts->code = DHO_DHCP_MESSAGE_TYPE;
  ack_opts->length = 1;
  ack_opts->val[0] = DHCPACK;

  // LEASE_TIME
  ack_opts = (dhcp_option_t*) (ack->options + 3);             // 3 bytes filled in prior
  ack_opts->code = DHO_DHCP_LEASE_TIME;
  ack_opts->length = 4;
  ack_opts->val[0] = (lease_ & 0xff000000) >> 24;
  ack_opts->val[1] = (lease_ & 0x00ff0000) >> 16;
  ack_opts->val[2] = (lease_ & 0x0000ff00) >> 8;
  ack_opts->val[3] = (lease_ & 0x000000ff);

  // SERVER_IDENTIFIER (the server's network address(es))
  ack_opts = (dhcp_option_t*) (ack->options + 9);             // 9 bytes filled in prior
  add_server_id(ack_opts);

  // MESSAGE (SHOULD)
  // No error message (only in DHCPNAK)
  ack_opts = (dhcp_option_t*) (ack->options + 15);            // 15 bytes filled in prior
  ack_opts->code = DHO_DHCP_MESSAGE;
  ack_opts->length = 1;
  ack_opts->val[0] = 0;

  // END
  ack_opts = (dhcp_option_t*) (ack->options + 18);            // 18 bytes filled in prior
  ack_opts->code   = DHO_END;
  ack_opts->length = 0;

  //printf("Sending a request ack\n");

  // If the giaddr field in a DHCP message from a client is non-zero, the server
  // sends any return messages to the DHCP server port on the BOOTP relay agent
  // whose address appears in giaddr
  // DHCP server port is port 67
  // DHCP client port is port 68
  if (msg->giaddr != IP4::addr{0}) {
    socket_.sendto(msg->giaddr, DHCP_SERVER_PORT, packet, packetlen);
    return;
  }

  // msg->giaddr is zero
  // If the giaddr field is zero and the ciaddr field is non-zero, then the server
  // unicasts DHCPOFFER and DHCPACK messages to the address in ciaddr
  if (msg->ciaddr != IP4::addr{0}) {
    socket_.sendto(msg->ciaddr, DHCP_CLIENT_PORT, packet, packetlen);
    return;
  }

  // msg->giaddr and msg->ciaddr is zero
  // If giaddr is zero and ciaddr is zero, and the broadcast bit (leftmost bit in
  // flags field) is set, then the server broadcasts DHCPOFFER and DHCPACK messsages
  // to 0xffffffff
  std::bitset<8> bits(msg->flags);
  if (bits[7] == 1) {
    socket_.bcast(server_id_, DHCP_CLIENT_PORT, packet, packetlen);
    return;
  }

  // the broadcast bit is not set
  // If the broadcast bit is not set and giaddr is zero and ciaddr is zero, then the
  // server unicasts DHCPOFFER and DHCPACK messages to the client's hardware address
  // and yiaddr address
  socket_.sendto(msg->yiaddr, DHCP_CLIENT_PORT, packet, packetlen);
  // TODO: Send the message to the client's hardware address:
  // socket_.sendto(, DHCP_CLIENT_PORT, packet, packetlen);
}

void DHCPD::nak(const dhcp_packet_t* msg) {

  // Server to client indicating client’s notion of network address is incorrect (e.g.,
  // client has moved to new subnet) or client’s lease has expired

  const size_t packetlen = sizeof(dhcp_packet_t);
  char packet[packetlen];

  // NAK
  dhcp_packet_t* nak = (dhcp_packet_t*) packet;

  nak->op = BOOTREPLY;
  nak->htype = 1; // From "Assigned Numbers" RFC. 1 = 10mb ethernet
  nak->hlen = 6;  // Hardware address length in octets. 6 for 10mb ethernet
  nak->hops = 0;
  nak->xid = msg->xid;
  nak->secs = 0;
  nak->ciaddr = 0;
  nak->yiaddr = 0;
  nak->siaddr = 0;
  nak->flags = msg->flags;
  nak->giaddr = msg->giaddr;
  std::memcpy(nak->chaddr, msg->chaddr, dhcp_packet_t::CHADDR_LEN);

  // OPTIONS
  dhcp_option_t* nak_opts = (dhcp_option_t*) (nak->options + 0);

  // MESSAGE_TYPE
  nak_opts->code = DHO_DHCP_MESSAGE_TYPE;
  nak_opts->length = 1;
  nak_opts->val[0] = DHCPNAK;

  // MESSAGE (SHOULD)
  nak_opts = (dhcp_option_t*) (nak->options + 3);         // 3 bytes filled in prior
  nak_opts->code = DHO_DHCP_MESSAGE;
  // TODO Fill in error message instead of 0
  nak_opts->length = 1;
  nak_opts->val[0] = 0;

  // SERVER_IDENTIFIER
  nak_opts = (dhcp_option_t*) (nak->options + 6);         // 6 bytes filled in prior
  add_server_id(nak_opts);

  // CLIENT_IDENTIFIER? (MAY)

  // END
  nak_opts = (dhcp_option_t*) (nak->options + 12);        // 12 bytes filled in prior
  nak_opts->code = DHO_END;
  nak_opts->length = 0;

  //printf("Sending DHCPNAK\n");

  // Send the DHCPNAK
  // Broadcast
  socket_.bcast(server_id_, DHCP_CLIENT_PORT, packet, packetlen);
}

const dhcp_option_t* DHCPD::get_option(const dhcp_option_t* opts, int code) const {
  const dhcp_option_t* opt = opts;
  while (opt->code != code && opt->code != DHO_END) {
    // go to next option
    opt = (const dhcp_option_t*) (((const uint8_t*) opt) + 2 + opt->length);
  }
  return opt;
}

Record::byte_seq DHCPD::get_client_id(const uint8_t* chaddr, const dhcp_option_t* opts) const {
  const dhcp_option_t* opt = get_option(opts, DHO_DHCP_CLIENT_IDENTIFIER);
  Record::byte_seq client_id;

  if (opt->code == DHO_DHCP_CLIENT_IDENTIFIER) {
    for (int i = 0; i < opt->length; i++)
      client_id.push_back(opt->val[i]);
    return client_id;
  }

  for (int i = 0; i < dhcp_packet_t::CHADDR_LEN; i++)
    client_id.push_back(chaddr[i]);

  return client_id;
}

IP4::addr DHCPD::get_requested_ip_in_opts(const dhcp_option_t* opts) const {
  const dhcp_option_t* opt = get_option(opts, DHO_DHCP_REQUESTED_ADDRESS);

  if (opt->code == DHO_DHCP_REQUESTED_ADDRESS)
    return IP4::addr{opt->val[0], opt->val[1], opt->val[2], opt->val[3]};

  return IP4::addr{0};
}

IP4::addr DHCPD::get_remote_netmask(const dhcp_option_t* opts) const {
  const dhcp_option_t* opt = get_option(opts, DHO_SUBNET_MASK);

  if (opt->code == DHO_SUBNET_MASK)
    return IP4::addr{opt->val[0], opt->val[1], opt->val[2], opt->val[3]};

  return IP4::addr{0};
}

void DHCPD::add_server_id(dhcp_option_t* opts) {
  opts->code = DHO_DHCP_SERVER_IDENTIFIER;
  opts->length = 4;
  opts->val[0] = server_id_.part(3);
  opts->val[1] = server_id_.part(2);
  opts->val[2] = server_id_.part(1);
  opts->val[3] = server_id_.part(0);
}

bool DHCPD::on_correct_network(IP4::addr giaddr, const dhcp_option_t* opts) const {

  // The client is on the wrong network if the result of applying the local subnet mask
  // or remote subnet mask (if giaddr is not zero) to requested IP address option value
  // doesn't match reality

  IP4::addr subnet = (giaddr == IP4::addr{0}) ? netmask_ : get_remote_netmask(opts);
  IP4::addr requested_ip = get_requested_ip_in_opts(opts);

  if (subnet == IP4::addr{0} or requested_ip == IP4::addr{0}) // Check if remote subnet mask from client exists and valid IP
    return false;

  // Check if requested IP is on the same subnet as the server
  if (network_address(requested_ip) == network_address(server_id_))
    return true;

  return false;
}
