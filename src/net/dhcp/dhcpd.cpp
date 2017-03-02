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
  for (auto& rec : records_) {
    if (rec.client_id() == client_id)
      return true;
  }
  return false;
}

int DHCPD::get_record_idx(const Record::byte_seq& client_id) const noexcept {
  for (size_t i = 0; i < records_.size(); i++) {
    if (records_.at(i).client_id() == client_id)
      return i;
  }
  return -1;
}

int DHCPD::get_record_idx_from_ip(IP4::addr ip) const noexcept {
  for (size_t i = 0; i < records_.size(); i++) {
    if (records_.at(i).ip() == ip)
      return i;
  }
  return -1;
}

bool DHCPD::valid_pool(IP4::addr start, IP4::addr end) const {
  if (start > end or start == end or inc_addr(start) == end)
      return false;

  // Check if start and end are on the same subnet and on the same subnet as the server
  if (network_address(start) != network_address(end)
    or network_address(start) != network_address(server_id_)
    or network_address(end) != network_address(server_id_))
    return false;

  return true;
}

void DHCPD::init_pool() {
  IP4::addr start = pool_start_;
  while (start < pool_end_ and network_address(start) == network_address(server_id_)) {
    pool_.emplace(std::make_pair(start, Status::AVAILABLE));
    start = inc_addr(start);
  }
  // Including pool_end_ IP:
  pool_.emplace(std::make_pair(pool_end_, Status::AVAILABLE));
}

void DHCPD::update_pool(IP4::addr ip, Status new_status) {
  auto it = pool_.find(ip);
  if (it not_eq pool_.end())
    it->second = new_status;
}

void DHCPD::listen() {
  socket_.on_read([&] (IP4::addr, UDP::port_t port,
    const char* data, size_t len) {

    if (port == DHCP_CLIENT_PORT) {
      if (len < sizeof(dhcp_packet_t))
        return;

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
  if (msg->op not_eq BOOTREQUEST or msg->hops not_eq 0) {
    debug("Invalid op\n");
    return;
  }

  if (msg->htype == 0 or msg->htype > HTYPE_HFI) {  // http://www.iana.org/assignments/arp-parameters/arp-parameters.xhtml
    debug("Invalid htype\n");
    return;
  }

  if (msg->yiaddr not_eq IP4::addr{0} or msg->siaddr not_eq IP4::addr{0} or msg->giaddr not_eq IP4::addr{0}) {
    debug("Invalid yiaddr, siaddr or giaddr\n");
    return;
  }

  if (not valid_options(opts)) {
    debug("Invalid options\n");
    return;
  }

  const auto cid = get_client_id(msg->chaddr, opts);
  if (cid.empty() or std::all_of(cid.begin(), cid.end(), [] (int i) { return i == 0; })) {
    debug("Invalid client id\n");
    return;
  }

  if (msg->magic[0] not_eq 99 or msg->magic[1] not_eq 130 or msg->magic[2] not_eq 83 or msg->magic[3] not_eq 99) {
    debug("Invalid magic\n");
    return;
  }

  int ridx;

  switch (opts->val[0]) {
    case DHCPDISCOVER:

      debug("------------Discover------------\n");
      print(msg, opts);

      offer(msg, opts);
      break;
    case DHCPREQUEST:

      debug("-------------Request--------------\n");
      print(msg, opts);

      handle_request(msg, opts);
      break;
    case DHCPDECLINE:

      debug("-------------Decline--------------\n");
      print(msg, opts);

      // A client has discovered that the suggested network address is already in use.
      // The server must mark the network address as not available and should notify the network admin
      // of a possible configuration problem - TODO
      ridx = get_record_idx(cid);
      // Erase record if exists because client hasn't got a valid IP
      if (ridx not_eq -1)
        records_.erase(records_.begin() + ridx);
      update_pool(get_requested_ip_in_opts(opts), Status::IN_USE);    // In use - possible config problem
      break;
    case DHCPRELEASE:

      debug("--------------Release--------------\n");
      print(msg, opts);

      // Mark the network address as not allocated anymore
      // Should retain a record of the client's initialization parameters for possible reuse
      // in response to subsequent requests from the client
      ridx = get_record_idx_from_ip(msg->ciaddr);
      if (ridx not_eq -1)
        records_.erase(records_.begin() + ridx);
      update_pool(msg->ciaddr, Status::AVAILABLE);
      break;

      // Or: Keep config parameters (record) and set status to RELEASED f.ex. instead (but need to handle this -
      // when a new discover (DHCPDISCOVER after DHCPRELEASE) is received - look through records and find record
      // with client id)
      // if (ridx not_eq -1)
      //    records_.at(ridx).set_status(Status::RELEASED);
      // update_pool(msg->ciaddr, Status::RELEASED);

    case DHCPINFORM:

      debug("---------------Inform---------------\n");
      print(msg, opts);

      inform_ack(msg);
      break;
  }
}

void DHCPD::handle_request(const dhcp_packet_t* msg, const dhcp_option_t* opts) {
  // Client could be:
  // 1. Responding to a DHCPOFFER message from a server
  // 2. Verifying a previously allocated IP address
  // 3. Extending the lease on a particular network address

  // Checking server identifier
  const dhcp_option_t* opt = get_option(opts, DHO_DHCP_SERVER_IDENTIFIER);
  // Server identifier from client:
  if (opt->code == DHO_DHCP_SERVER_IDENTIFIER) {  // Then this is a response to a DHCPOFFER message
    IP4::addr sid{opt->val[0], opt->val[1], opt->val[2], opt->val[3]};

    if (sid not_eq server_id()) { // The client has not chosen this server

      debug("Client hasn't chosen this server - returning\n");

      int ridx = get_record_idx(get_client_id(msg->chaddr, opts));
      if (ridx != -1) {

        debug("Freeing up record\n");

        // Free up the IP address - the client has declined our offer
        update_pool(records_.at(ridx).ip(), Status::AVAILABLE);
        records_.erase(records_.begin() + ridx);
      }
      return;
    }

    // Server identifier == this server's ID

    debug("Client has chosen this server\n");

    int ridx = get_record_idx(get_client_id(msg->chaddr, opts));

    if (ridx not_eq -1) { // A record for this client already exists
      auto& record = records_.at(ridx);

      // If ciaddr is zero and requested IP address is filled in with the yiaddr value from the chosen DHCPOFFER:
      // Client state: SELECTING
      if (msg->ciaddr == IP4::addr{0} and get_requested_ip_in_opts(opts) == record.ip()) {
        // RECORD
        record.set_status(Status::IN_USE);
        // POOL
        update_pool(record.ip(), Status::IN_USE);
        // Send DHCPACK
        request_ack(msg, opts);
        return;
      }

      debug("ciaddr not zero or not correct requested ip - sending nak\n");

      // Else ciaddr is not zero or record's ip is not the same as the requested ip in the request
      // TODO: nak() or silent return?
      nak(msg);
      return;
    }

    debug("No record of this client exists - sending nak\n");

    // Else a record with this client id doesn't exist
    // TODO: nak() or silent return?
    nak(msg);
    return;
  }

  debug("Verifying or extending lease\n");

  // Else server identifier is not filled in and this is not a response to a DHCPOFFER message, but
  // a request to verify or extend an existing lease
  verify_or_extend_lease(msg, opts);
}

void DHCPD::verify_or_extend_lease(const dhcp_packet_t* msg, const dhcp_option_t* opts) {
  int ridx = get_record_idx(get_client_id(msg->chaddr, opts));

  if (ridx == -1) {
    // If the server has no record of this client, then it MUST remain silent and MAY
    // output a warning to the network administrator.
    return;
  }

  if (msg->ciaddr == IP4::addr{0}) {
    // Then the client is seeking to verify a previously allocated, cached configuration
    // Client state: INIT-REBOOT

    debug("Init-reboot\n");

    // 1. (Before or in the if statement) Check if on correct network
    // Server should send a DHCPNAK if the client is on the wrong network
    // If the server detects that the client is on the wrong net (i.e., the result of applying the local
    // subnet mask or remote subnet mask (if giaddr is not zero) to requested IP address value doesn't
    // match reality), then the server should send a DHCPNAK message to the client.
    // 2. (After or in the if statement) If client is on correct network: Check if the client's notion of IP is correct
    if ((not on_correct_network(msg->giaddr, opts)) or (get_requested_ip_in_opts(opts) not_eq records_.at(ridx).ip())) {

      debug("Client not on correct network\n");

      // Updating pool and erasing record
      update_pool(records_.at(ridx).ip(), Status::AVAILABLE);
      records_.erase(records_.begin() + ridx);
      nak(msg);
      return;
    }

    // Else everything is OK and we want to verify the client's configuration
    // Update pool and record too
    auto& record = records_.at(ridx);
    record.set_status(Status::IN_USE);
    record.set_lease_start(RTC::now());
    update_pool(record.ip(), Status::IN_USE);

    // Sending DHCPACK
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
    // has been unicast to the server or not. If zero: Unicast. If non-zero: Broadcast.
    // REBINDING: The client is completely configured and is trying to extend its lease. This message MUST
    // be broadcast to the 0xffffffff IP broadcast address. The server should check ciaddr for correctness
    // before replying to the DHCPREQUEST

    // Table 4 page 33 in RFC 2131 - Fields in client message from client in different state

    debug("Renewing or rebinding state\n");

    // Extend the lease or not - should send a DHCPACK regardless
    // Update record and pool
    auto& record = records_.at(ridx);
    record.set_status(Status::IN_USE);
    record.set_lease_start(RTC::now());
    update_pool(record.ip(), Status::IN_USE);

    // Send DHCPACK
    request_ack(msg, opts);
  }
}

void DHCPD::offer(const dhcp_packet_t* msg, const dhcp_option_t* opts) {
  // It may occur that the client doesn't get our offers and then we don't want to
  // exhaust our pool
  int ridx = get_record_idx(get_client_id(msg->chaddr, opts));
  if (ridx not_eq -1)
    return;

  char packet[PACKET_SIZE];

  // If we want to offer this client an IP address: Create a RECORD
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

  // yiaddr - IP address offered to the client from the pool
  bool ok = false;
  for (auto& entry : pool_) {
    if (entry.second == Status::AVAILABLE) {

      debug("Found available IP: %s\n", entry.first.to_string().c_str());

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
  if (not ok) { // Then no IP is available - sending DHCPNAK

    debug("No more available IPs - clearing offered ips and sending nak\n");

    // Clear offered ips that have not been confirmed - Maybe use pending value instead
    clear_offered_ips();
    nak(msg);   // Record has not been added to the records_ vector so OK
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
  dhcp_option_t* offer_opts = conv_option(offer->options, 0);

  // DHCPOFFER Options:
  // Requested IP address                                         MUST NOT
  // IP address lease time                                        MUST
  // Use file/sname fields                                        MAY
  // DHCP message type                                            DHCPOFFER
  // Parameter request list (DHO_DHCP_PARAMETER_REQUEST_LIST)     MUST NOT
  // Message (DHO_DHCP_MESSAGE)                                   SHOULD
  // Client identifier                                            MUST NOT - note RFC 6842
  // Vendor class identifier                                      MAY
  // Server identifier                                            MUST
  // Maximum message size                                         MUST NOT
  // All others                                                   MAY

  // MESSAGE_TYPE
  offer_opts->code = DHO_DHCP_MESSAGE_TYPE;
  offer_opts->length = 1;
  offer_opts->val[0] = DHCPOFFER;

  // SERVER_IDENTIFIER (the server's network address(es))
  offer_opts = conv_option(offer->options, 3);     // 3 bytes filled in prior
  add_server_id(offer_opts);

  // SUBNET_MASK
  offer_opts = conv_option(offer->options, 9);     // 9 bytes filled in prior
  offer_opts->code = DHO_SUBNET_MASK;
  offer_opts->length = 4;
  offer_opts->val[0] = netmask_.part(3);
  offer_opts->val[1] = netmask_.part(2);
  offer_opts->val[2] = netmask_.part(1);
  offer_opts->val[3] = netmask_.part(0);

  // ROUTERS
  offer_opts = conv_option(offer->options, 15);      // 15 bytes filled in prior
  offer_opts->code = DHO_ROUTERS;
  offer_opts->length = 4;
  offer_opts->val[0] = router_.part(3);
  offer_opts->val[1] = router_.part(2);
  offer_opts->val[2] = router_.part(1);
  offer_opts->val[3] = router_.part(0);

  // LEASE_TIME
  // Time in units of seconds (32-bit unsigned integer / 4 uint8_t)
  offer_opts = conv_option(offer->options, 21);      // 21 bytes filled in prior
  offer_opts->code = DHO_DHCP_LEASE_TIME;
  offer_opts->length = 4;
  offer_opts->val[0] = (lease_ & 0xff000000) >> 24;
  offer_opts->val[1] = (lease_ & 0x00ff0000) >> 16;
  offer_opts->val[2] = (lease_ & 0x0000ff00) >> 8;
  offer_opts->val[3] = (lease_ & 0x000000ff);

  // MESSAGE (SHOULD)
  // No error message (only in DHCPNAK)
  offer_opts = conv_option(offer->options, 27);      // 27 bytes filled in prior
  offer_opts->code = DHO_DHCP_MESSAGE;
  offer_opts->length = 1;
  offer_opts->val[0] = 0;

  // TIME_OFFSET
  offer_opts = conv_option(offer->options, 30);
  offer_opts->code = DHO_TIME_OFFSET;
  offer_opts->length = 4;
  offer_opts->val[0] = 0;
  offer_opts->val[1] = 0;
  offer_opts->val[2] = 0;
  offer_opts->val[3] = 0;

  // DOMAIN_NAME_SERVERS
  offer_opts = conv_option(offer->options, 36);
  offer_opts->code = DHO_DOMAIN_NAME_SERVERS;
  offer_opts->length = 4;
  offer_opts->val[0] = dns_.part(3);
  offer_opts->val[1] = dns_.part(2);
  offer_opts->val[2] = dns_.part(1);
  offer_opts->val[3] = dns_.part(0);

  // BROADCAST_ADDRESS
  offer_opts = conv_option(offer->options, 42);
  offer_opts->code = DHO_BROADCAST_ADDRESS;
  offer_opts->length = 4;
  auto broadcast = broadcast_address();
  offer_opts->val[0] = broadcast.part(3);
  offer_opts->val[1] = broadcast.part(2);
  offer_opts->val[2] = broadcast.part(1);
  offer_opts->val[3] = broadcast.part(0);

  // END
  offer_opts = conv_option(offer->options, 48);
  offer_opts->code   = DHO_END;
  offer_opts->length = 0;

  // RECORD
  add_record(record);

  debug("Sending offer\n");

  // If the giaddr field in a DHCP message from a client is non-zero, the server sends any return
  // messages to the DHCP server port on the BOOTP relay agent whose address appears in giaddr
  if (msg->giaddr != IP4::addr{0}) {
    socket_.sendto(msg->giaddr, DHCP_SERVER_PORT, packet, PACKET_SIZE);
    return;
  }
  // If the giaddr field is zero and the ciaddr field is non-zero, then the server unicasts
  // DHCPOFFER and DHCPACK messages to the address in ciaddr
  if (msg->ciaddr != IP4::addr{0}) {
    socket_.sendto(msg->ciaddr, DHCP_CLIENT_PORT, packet, PACKET_SIZE);
    return;
  }
  // If giaddr is zero and ciaddr is zero, and the broadcast bit (leftmost bit in flags field) is set,
  // then the server broadcasts DHCPOFFER and DHCPACK messsages to 0xffffffff
  if (msg->flags & BOOTP_BROADCAST) {
    socket_.bcast(server_id_, DHCP_CLIENT_PORT, packet, PACKET_SIZE);
    return;
  }
  // If the broadcast bit is not set and giaddr is zero and ciaddr is zero, then the server unicasts
  // DHCPOFFER and DHCPACK messages to the client's hardware address and yiaddr address
  socket_.sendto(msg->yiaddr, DHCP_CLIENT_PORT, packet, PACKET_SIZE);
  // TODO: Send the message to the client's hardware address:
  // socket_.sendto(, DHCP_CLIENT_PORT, packet, PACKET_SIZE);
}

void DHCPD::inform_ack(const dhcp_packet_t* msg) {
  // The client has obtained a network address through some other means (e.g., manual configuration)
  // and may use a DHCPINFORM request message to obtain other local configuration parameters.
  // Servers receiving a DHCPINFORM message construct a DHCPACK message with any local configuration
  // parameters appropriate for the client without:
  // - allocating a new address
  // - checking for an existing binding
  // - filling in yiaddr
  // - including lease time parameters
  // The servers should unicast the DHCPACK reply to the address given in the ciaddr field of the
  // DHCPINFORM message.

  // The server should check the network address in the DHCPINFORM message for consistency, but must not
  // check for an existing lease. The server forms a DHCPACK message containing the configuration parameters
  // for the requesting client and sends the DHCPACK message directly to the client.

  char packet[PACKET_SIZE];

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
  dhcp_option_t* ack_opts = conv_option(ack->options, 0);

  // MESSAGE_TYPE
  ack_opts->code = DHO_DHCP_MESSAGE_TYPE;
  ack_opts->length = 1;
  ack_opts->val[0] = DHCPACK;

  debug("Sending inform ack\n");

  // TODO Add options requested by the client

  // Send the DHCPACK - Unicast to DHCPINFORM message's ciaddr field
  socket_.sendto(msg->ciaddr, DHCP_CLIENT_PORT, packet, PACKET_SIZE);
}

void DHCPD::request_ack(const dhcp_packet_t* msg, const dhcp_option_t* opts) {
  // If the client included a list of requested parameters in a DHCPDISCOVER message, it MUST
  // include that list in all subsequent messages.

  // Any configuration parameters in the DHCPACK message SHOULD NOT conflict with those in the earlier
  // DHCPOFFER message to which the client is responding.
  // DHCPACK: Server to client with configuration parameters, including committed network address

  // Table 3 RFC 2131 DHCPACK

  char packet[PACKET_SIZE];

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
  dhcp_option_t* ack_opts = conv_option(ack->options, 0);

  // MESSAGE_TYPE
  ack_opts->code = DHO_DHCP_MESSAGE_TYPE;
  ack_opts->length = 1;
  ack_opts->val[0] = DHCPACK;

  // LEASE_TIME (MUST)
  ack_opts = conv_option(ack->options, 3);     // 3 bytes filled in prior
  ack_opts->code = DHO_DHCP_LEASE_TIME;
  ack_opts->length = 4;
  ack_opts->val[0] = (lease_ & 0xff000000) >> 24;
  ack_opts->val[1] = (lease_ & 0x00ff0000) >> 16;
  ack_opts->val[2] = (lease_ & 0x0000ff00) >> 8;
  ack_opts->val[3] = (lease_ & 0x000000ff);

  // SERVER_IDENTIFIER (the server's network address(es))
  ack_opts = conv_option(ack->options, 9);     // 9 bytes filled in prior
  add_server_id(ack_opts);

  // MESSAGE (SHOULD)
  // No error message (only in DHCPNAK)
  ack_opts = conv_option(ack->options, 15);    // 15 bytes filled in prior
  ack_opts->code = DHO_DHCP_MESSAGE;
  ack_opts->length = 1;
  ack_opts->val[0] = 0;

  // END
  ack_opts = conv_option(ack->options, 18);    // 18 bytes filled in prior
  ack_opts->code   = DHO_END;
  ack_opts->length = 0;

  debug("Sending request ack\n");

  // If the giaddr field in a DHCP message from a client is non-zero, the server sends any return
  // messages to the DHCP server port on the BOOTP relay agent whose address appears in giaddr
  if (msg->giaddr != IP4::addr{0}) {
    socket_.sendto(msg->giaddr, DHCP_SERVER_PORT, packet, PACKET_SIZE);
    return;
  }
  // If the giaddr field is zero and the ciaddr field is non-zero, then the server unicasts
  // DHCPOFFER and DHCPACK messages to the address in ciaddr
  if (msg->ciaddr != IP4::addr{0}) {
    socket_.sendto(msg->ciaddr, DHCP_CLIENT_PORT, packet, PACKET_SIZE);
    return;
  }
  // If giaddr is zero and ciaddr is zero, and the broadcast bit (leftmost bit in flags field) is set,
  // then the server broadcasts DHCPOFFER and DHCPACK messsages to 0xffffffff
  std::bitset<8> bits(msg->flags);
  if (bits[7] == 1) {
    socket_.bcast(server_id_, DHCP_CLIENT_PORT, packet, PACKET_SIZE);
    return;
  }
  // If the broadcast bit is not set and giaddr is zero and ciaddr is zero, then the server unicasts
  // DHCPOFFER and DHCPACK messages to the client's hardware address and yiaddr address
  socket_.sendto(msg->yiaddr, DHCP_CLIENT_PORT, packet, PACKET_SIZE);
  // TODO: Send the message to the client's hardware address:
  // socket_.sendto(, DHCP_CLIENT_PORT, packet, PACKET_SIZE);
}

void DHCPD::nak(const dhcp_packet_t* msg) {
  // DHCPNAK: Server to client indicating client’s notion of network address is incorrect (e.g., client
  // has moved to new subnet) or client’s lease has expired

  char packet[PACKET_SIZE];

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
  dhcp_option_t* nak_opts = conv_option(nak->options, 0);

  // MESSAGE_TYPE
  nak_opts->code = DHO_DHCP_MESSAGE_TYPE;
  nak_opts->length = 1;
  nak_opts->val[0] = DHCPNAK;

  // MESSAGE (SHOULD)
  nak_opts = conv_option(nak->options, 3);   // 3 bytes filled in prior
  nak_opts->code = DHO_DHCP_MESSAGE;
  // TODO Fill in error message instead of 0
  nak_opts->length = 1;
  nak_opts->val[0] = 0;

  // SERVER_IDENTIFIER
  nak_opts = conv_option(nak->options, 6);   // 6 bytes filled in prior
  add_server_id(nak_opts);

  // END
  nak_opts = conv_option(nak->options, 12);  // 12 bytes filled in prior
  nak_opts->code = DHO_END;
  nak_opts->length = 0;

  debug("Sending NAK\n");

  // Send the DHCPNAK - Broadcast
  socket_.bcast(server_id_, DHCP_CLIENT_PORT, packet, PACKET_SIZE);
}

bool DHCPD::valid_options(const dhcp_option_t* opts) const {
  int num_bytes = 0;
  int num_options = 0;

  const dhcp_option_t* opt = opts;
  while (opt->code != DHO_END) {
    num_bytes += 2 + opt->length;
    num_options++;

    // F.ex. if DHO_END isn't included
    if (num_bytes > DHCP_VEND_LEN or num_options > MAX_NUM_OPTIONS)
      return false;

    // go to next option
    opt = (const dhcp_option_t*) (((const uint8_t*) opt) + 2 + opt->length);
  }

  return true;
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
  // The client is on the wrong network if the result of applying the local subnet mask or remote subnet
  // mask (if giaddr is not zero) to requested IP address option value doesn't match reality

  IP4::addr subnet = (giaddr == IP4::addr{0}) ? netmask_ : get_remote_netmask(opts);
  IP4::addr requested_ip = get_requested_ip_in_opts(opts);

  // Check if remote subnet mask from client exists and valid IP
  if (subnet == IP4::addr{0} or requested_ip == IP4::addr{0})
    return false;

  // Check if requested IP is on the same subnet as the server
  if (network_address(requested_ip) == network_address(server_id_))
    return true;

  return false;
}

void DHCPD::clear_offered_ip(IP4::addr ip) {
  int ridx = get_record_idx_from_ip(ip);
  if (ridx not_eq -1)
    records_.erase(records_.begin() + ridx);
  update_pool(ip, Status::AVAILABLE);
}

void DHCPD::clear_offered_ips() {
  // looping through pool
  for (auto& entry : pool_) {
    if (entry.second == Status::OFFERED)
      entry.second = Status::AVAILABLE;
  }

  // and records
  for (size_t i = 0; i < records_.size(); i++) {
    if (records_.at(i).status() == Status::OFFERED) {
      // Update pool
      // update_pool(records_.at(i).ip(), Status::AVAILABLE);
      // And erase record
      records_.erase(records_.begin() + i);
    }
  }
}
