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
#include <net/inet>
#include <rtc>

using namespace net;
using namespace dhcp;

DHCPD::DHCPD(UDP& udp, ip4::Addr pool_start, ip4::Addr pool_end,
  uint32_t lease, uint32_t max_lease, uint8_t pending)
: stack_{udp.stack()},
  socket_{udp.bind(DHCP_SERVER_PORT)},
  pool_start_{pool_start},
  pool_end_{pool_end},
  server_id_{stack_.ip_addr()},
  netmask_{stack_.netmask()},
  router_{stack_.gateway()},
  dns_{stack_.dns_addr()},
  lease_{lease}, max_lease_{max_lease}, pending_{pending}
{
  if (not valid_pool(pool_start, pool_end))
    throw DHCP_exception{"Invalid pool"};

  init_pool();
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

int DHCPD::get_record_idx_from_ip(ip4::Addr ip) const noexcept {
  for (size_t i = 0; i < records_.size(); i++) {
    if (records_.at(i).ip() == ip)
      return i;
  }
  return -1;
}

bool DHCPD::valid_pool(ip4::Addr start, ip4::Addr end) const {
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
  ip4::Addr start = pool_start_;
  while (start < pool_end_ and network_address(start) == network_address(server_id_)) {
    pool_.emplace(std::make_pair(start, Status::AVAILABLE));
    start = inc_addr(start);
  }
  // Including pool_end_ IP:
  pool_.emplace(std::make_pair(pool_end_, Status::AVAILABLE));
}

void DHCPD::update_pool(ip4::Addr ip, Status new_status) {
  auto it = pool_.find(ip);
  if (it not_eq pool_.end())
    it->second = new_status;
}

void DHCPD::listen() {
  socket_.on_read(
  [this] (net::Addr, UDP::port_t port,
          const char* data, size_t len)
  {
    if (port == DHCP_CLIENT_PORT) {
      if (len < sizeof(Message))
        return;

      // Message
      const auto* msg = reinterpret_cast<const Message*>(data);
      // Find out what kind of DHCP message this is
      resolve(msg);
    }
  });
}

void DHCPD::resolve(const Message* msg) {
  if (UNLIKELY(msg->op not_eq static_cast<uint8_t>(op_code::BOOTREQUEST)
      or msg->hops not_eq 0))
  {
    debug("Invalid op\n");
    return;
  }

  if (UNLIKELY(msg->htype == 0 or msg->htype > static_cast<uint8_t>(htype::HFI))) {  // http://www.iana.org/assignments/arp-parameters/arp-parameters.xhtml
    debug("Invalid htype\n");
    return;
  }

  if (UNLIKELY(msg->yiaddr not_eq 0 or msg->siaddr not_eq 0 or msg->giaddr not_eq 0)) {
    debug("Invalid yiaddr, siaddr or giaddr\n");
    return;
  }

  if (not valid_options(msg)) {
    debug("Invalid options\n");
    return;
  }

  const auto cid = get_client_id(msg);
  if (std::all_of(cid.begin(), cid.end(), [] (int i) { return i == 0; })) {
    debug("Invalid client id\n");
    return;
  }

  if (msg->magic[0] not_eq 99 or msg->magic[1] not_eq 130 or msg->magic[2] not_eq 83 or msg->magic[3] not_eq 99) {
    debug("Invalid magic\n");
    return;
  }

  const Message_reader reader{msg};
  const auto* msg_opt = reader.find_option<option::message_type>();

  if (UNLIKELY(msg_opt == nullptr)) {
    debug("No Message option\n");
    return;
  }

  int ridx;
  switch(msg_opt->type()) {

    case message_type::DISCOVER:

      debug("------------Discover------------\n");
      print(msg);

      offer(msg);
      break;
    case message_type::REQUEST:

      debug("-------------Request--------------\n");
      print(msg);

      handle_request(msg);
      break;
    case message_type::DECLINE:

      debug("-------------Decline--------------\n");
      print(msg);

      // A client has discovered that the suggested network address is already in use.
      // The server must mark the network address as not available and should notify the network admin
      // of a possible configuration problem - TODO
      ridx = get_record_idx(cid);
      // Erase record if exists because client hasn't got a valid IP
      if (ridx not_eq -1)
        records_.erase(records_.begin() + ridx);
      update_pool(get_requested_ip_in_opts(msg), Status::IN_USE);    // In use - possible config problem
      return;
    case message_type::RELEASE:

      debug("--------------Release--------------\n");
      print(msg);

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

    case message_type::INFORM:

      debug("---------------Inform---------------\n");
      print(msg);

      inform_ack(msg);
      break;

    default:
      debug("---------------Unhandled------------\n")
      print(msg);

      break;
  }
}

void DHCPD::handle_request(const Message* msg) {
  // Client could be:
  // 1. Responding to a DHCPOFFER message from a server
  // 2. Verifying a previously allocated IP address
  // 3. Extending the lease on a particular network address

  const Message_reader reader{msg};

  // Checking server identifier
  const auto* opt = reader.find_option<option::server_identifier>();
  // Server identifier from client:
  if (opt != nullptr) {  // Then this is a response to a DHCPOFFER message
    ip4::Addr sid = *(opt->addr<ip4::Addr>());

    if (sid not_eq server_id()) { // The client has not chosen this server

      debug("Client hasn't chosen this server - returning\n");

      int ridx = get_record_idx(get_client_id(msg));
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

    int ridx = get_record_idx(get_client_id(msg));

    if (ridx not_eq -1) { // A record for this client already exists
      auto& record = records_.at(ridx);

      // If ciaddr is zero and requested IP address is filled in with the yiaddr value from the chosen DHCPOFFER:
      // Client state: SELECTING
      if (msg->ciaddr == ip4::Addr{0} and get_requested_ip_in_opts(msg) == record.ip()) {
        // RECORD
        record.set_status(Status::IN_USE);
        // POOL
        update_pool(record.ip(), Status::IN_USE);
        // Send DHCPACK
        request_ack(msg);
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
  verify_or_extend_lease(msg);
}

void DHCPD::verify_or_extend_lease(const Message* msg) {
  int ridx = get_record_idx(get_client_id(msg));

  if (ridx == -1) {
    // If the server has no record of this client, then it MUST remain silent and MAY
    // output a warning to the network administrator.
    return;
  }

  if (msg->ciaddr == ip4::Addr{0}) {
    // Then the client is seeking to verify a previously allocated, cached configuration
    // Client state: INIT-REBOOT

    debug("Init-reboot\n");

    // 1. (Before or in the if statement) Check if on correct network
    // Server should send a DHCPNAK if the client is on the wrong network
    // If the server detects that the client is on the wrong net (i.e., the result of applying the local
    // subnet mask or remote subnet mask (if giaddr is not zero) to requested IP address value doesn't
    // match reality), then the server should send a DHCPNAK message to the client.
    // 2. (After or in the if statement) If client is on correct network: Check if the client's notion of IP is correct
    if ((not on_correct_network(msg)) or (get_requested_ip_in_opts(msg) not_eq records_.at(ridx).ip())) {

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
    request_ack(msg);
    return;
  }

  if (get_requested_ip_in_opts(msg) == ip4::Addr{0} and msg->ciaddr not_eq ip4::Addr{0}) {
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
    request_ack(msg);
  }
}

void DHCPD::offer(const Message* msg) {
  // It may occur that the client doesn't get our offers and then we don't want to
  // exhaust our pool
  auto client_id = get_client_id(msg);
  int ridx = get_record_idx(client_id);
  if (ridx not_eq -1)
    return;

  // Search for available IP in pool
  auto it = std::find_if(pool_.begin(), pool_.end(), [](const auto& entry)->bool {
    return entry.second == Status::AVAILABLE;
  });

  if(UNLIKELY(it == pool_.end()))
  {
    debug("No more available IPs - clearing offered ips and sending nak\n");

    // Clear offered ips that have not been confirmed - Maybe use pending value instead
    clear_offered_ips();
    nak(msg);   // Record has not been added to the records_ vector so OK
    return;
  }

  const auto free_addr = it->first;
  debug("Found available IP: %s\n", free_addr.to_string().c_str());
  // mark the IP as offered
  it->second = Status::OFFERED;

  // If we want to offer this client an IP address: Create a RECORD
  Record record;
  // Keep client identifier (index into database)
  record.set_client_id(client_id);

  // Servers need not reserve the offered network address, although the protocol will work more efficiently if the server
  // avoids allocating the offered network address to another client.
  // When allocating a new address, servers should check that the offered network address is not already in use; e.g.
  // the server may probe the offered address with an ICMP Echo Request. Servers SHOULD be implemented so that
  // network admins MAY choose to disable probes of newly allocated addresses.

  // RECORD
  record.set_ip(free_addr);
  record.set_status(Status::OFFERED);
  record.set_lease_start(RTC::now());
  record.set_lease_duration(lease_);

  // OFFER
  // NOTE: It may be possible with some changes to use the incoming "msg" as buffer to avoid some extra copy
  // due to some of the fields are sent in return to the client
  char buffer[Message::size()];
  Message_writer offer{reinterpret_cast<Message*>(buffer), op_code::BOOTREPLY, message_type::OFFER};

  offer.set_hw_addr(htype::ETHER, sizeof(MAC::Addr)); // assume ethernet
  offer.set_xid(ntohl(msg->xid));
  //offer.set_ciaddr(ip4::Addr{0});
  //offer.set_siaddr(ip4::Addr{0});   // IP address of next bootstrap server
  // yiaddr - IP address offered to the client from the pool
  offer.set_yiaddr(free_addr);

  offer.set_flags(msg->flags);
  offer.set_giaddr(msg->giaddr);
  offer.set_chaddr(reinterpret_cast<const MAC::Addr*>(&msg->chaddr)); // assume ethernet

  offer.set_sname(msg->sname);  // Server host name or options - MAY
  offer.set_file(msg->file);    // Client boot file name or options - MAY

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

  // SERVER_IDENTIFIER (the server's network address(es))
  offer.add_option<option::server_identifier>(&server_id_);

  // SUBNET_MASK
  offer.add_option<option::subnet_mask>(&netmask_);

  // ROUTERS
  offer.add_option<option::routers>(&router_);

  // LEASE_TIME
  offer.add_option<option::lease_time>(lease_);

  // TIME_OFFSET
  offer.add_option<option::time_offset>(0);

  // DOMAIN_NAME_SERVERS
  offer.add_option<option::domain_name_servers>(&dns_);

  // BROADCAST_ADDRESS
  auto bcast = broadcast_address();
  offer.add_option<option::broadcast_address>(&bcast);

  // END
  offer.end();

  // RECORD
  add_record(record);

  debug("Sending offer\n");

  // If the giaddr field in a DHCP message from a client is non-zero, the server sends any return
  // messages to the DHCP server port on the BOOTP relay agent whose address appears in giaddr
  if (msg->giaddr != ip4::Addr{0}) {
    socket_.sendto(msg->giaddr, DHCP_SERVER_PORT, buffer, sizeof(buffer));
    return;
  }
  // If the giaddr field is zero and the ciaddr field is non-zero, then the server unicasts
  // DHCPOFFER and DHCPACK messages to the address in ciaddr
  if (msg->ciaddr != ip4::Addr{0}) {
    socket_.sendto(msg->ciaddr, DHCP_CLIENT_PORT, buffer, sizeof(buffer));
    return;
  }
  // If giaddr is zero and ciaddr is zero, and the broadcast bit (leftmost bit in flags field) is set,
  // then the server broadcasts DHCPOFFER and DHCPACK messsages to 0xffffffff
  if (ntohs(msg->flags) & static_cast<uint16_t>(flag::BOOTP_BROADCAST)) {
    socket_.bcast(server_id_, DHCP_CLIENT_PORT, buffer, sizeof(buffer));
    return;
  }
  // If the broadcast bit is not set and giaddr is zero and ciaddr is zero, then the server unicasts
  // DHCPOFFER and DHCPACK messages to the client's hardware address and yiaddr address
  socket_.sendto(msg->yiaddr, DHCP_CLIENT_PORT, buffer, sizeof(buffer));
  // TODO: Send the message to the client's hardware address:
  // socket_.sendto(, DHCP_CLIENT_PORT, packet, PACKET_SIZE);
}

void DHCPD::inform_ack(const Message* msg) {
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

  char buffer[Message::size()];
  Message_writer ack{reinterpret_cast<Message*>(buffer), op_code::BOOTREPLY, message_type::ACK};

  ack.set_hw_addr(htype::ETHER, sizeof(MAC::Addr)); // assume ethernet
  ack.set_xid(ntohl(msg->xid));
  ack.set_ciaddr(msg->ciaddr); // or 0
  ack.set_giaddr(msg->giaddr);
  ack.set_flags(msg->flags);
  ack.set_chaddr(reinterpret_cast<const MAC::Addr*>(&msg->chaddr)); // assume ethernet
  ack.set_sname(msg->sname);  // Server host name or options - MAY
  ack.set_file(msg->file);    // Client boot file name or options - MAY

  debug("Sending inform ack\n");

  // TODO Add options requested by the client

  // Send the DHCPACK - Unicast to DHCPINFORM message's ciaddr field
  socket_.sendto(msg->ciaddr, DHCP_CLIENT_PORT, buffer, sizeof(buffer));
}

void DHCPD::request_ack(const Message* msg) {
  // If the client included a list of requested parameters in a DHCPDISCOVER message, it MUST
  // include that list in all subsequent messages.

  // Any configuration parameters in the DHCPACK message SHOULD NOT conflict with those in the earlier
  // DHCPOFFER message to which the client is responding.
  // DHCPACK: Server to client with configuration parameters, including committed network address

  // Table 3 RFC 2131 DHCPACK

  char buffer[Message::size()];
  Message_writer ack{reinterpret_cast<Message*>(buffer), op_code::BOOTREPLY, message_type::ACK};

  ack.set_hw_addr(htype::ETHER, sizeof(MAC::Addr)); // assume ethernet
  ack.set_xid(ntohl(msg->xid));

  int ridx = get_record_idx(get_client_id(msg));
  ack.set_yiaddr((ridx not_eq -1) ? records_.at(ridx).ip() : 0); // TODO: else what?     // IP address assigned to client
  ack.set_ciaddr(msg->ciaddr); // or 0
  ack.set_giaddr(msg->giaddr);

  ack.set_flags(msg->flags);
  ack.set_chaddr(reinterpret_cast<const MAC::Addr*>(&msg->chaddr)); // assume ethernet
  ack.set_sname(msg->sname);  // Server host name or options - MAY
  ack.set_file(msg->file);    // Client boot file name or options - MAY

  // LEASE_TIME (MUST)
  ack.add_option<option::lease_time>(lease_);

  // SERVER_IDENTIFIER (the server's network address(es))
  ack.add_option<option::server_identifier>(&server_id_);

  // MESSAGE (SHOULD)
  // No error message (only in DHCPNAK)
  //ack.add_option<option::message>(message_type::NO_ERROR);
  //ack_opts = conv_option(ack->options, 15);    // 15 bytes filled in prior
  //ack_opts->code = DHO_DHCP_MESSAGE;
  //ack_opts->length = 1;
  //ack_opts->val[0] = 0;

  // END
  ack.end();

  debug("Sending request ack\n");

  // If the giaddr field in a DHCP message from a client is non-zero, the server sends any return
  // messages to the DHCP server port on the BOOTP relay agent whose address appears in giaddr
  if (msg->giaddr != ip4::Addr{0}) {
    socket_.sendto(msg->giaddr, DHCP_SERVER_PORT, buffer, sizeof(buffer));
    return;
  }
  // If the giaddr field is zero and the ciaddr field is non-zero, then the server unicasts
  // DHCPOFFER and DHCPACK messages to the address in ciaddr
  if (msg->ciaddr != ip4::Addr{0}) {
    socket_.sendto(msg->ciaddr, DHCP_CLIENT_PORT, buffer, sizeof(buffer));
    return;
  }
  // If giaddr is zero and ciaddr is zero, and the broadcast bit (leftmost bit in flags field) is set,
  // then the server broadcasts DHCPOFFER and DHCPACK messsages to 0xffffffff
  if (ntohs(msg->flags) & static_cast<uint16_t>(flag::BOOTP_BROADCAST)) {
    socket_.bcast(server_id_, DHCP_CLIENT_PORT, buffer, sizeof(buffer));
    return;
  }
  // If the broadcast bit is not set and giaddr is zero and ciaddr is zero, then the server unicasts
  // DHCPOFFER and DHCPACK messages to the client's hardware address and yiaddr address
  socket_.sendto(msg->yiaddr, DHCP_CLIENT_PORT, buffer, sizeof(buffer));
  // TODO: Send the message to the client's hardware address:
  // socket_.sendto(, DHCP_CLIENT_PORT, packet, PACKET_SIZE);
}

void DHCPD::nak(const Message* msg) {
  // DHCPNAK: Server to client indicating client’s notion of network address is incorrect (e.g., client
  // has moved to new subnet) or client’s lease has expired

  // NAK
  char buffer[Message::size()];
  Message_writer nak{reinterpret_cast<Message*>(buffer), op_code::BOOTREPLY, message_type::NAK};

  nak.set_hw_addr(htype::ETHER, sizeof(MAC::Addr)); // assume ethernet
  nak.set_xid(ntohl(msg->xid));

  nak.set_giaddr(msg->giaddr);

  nak.set_flags(msg->flags);
  nak.set_chaddr(reinterpret_cast<const MAC::Addr*>(&msg->chaddr)); // assume ethernet

  // MESSAGE (SHOULD)
  nak.add_option<option::message>(std::string{"Something went wrong"});

  // SERVER_IDENTIFIER
  nak.add_option<option::server_identifier>(&server_id_);

  // END
  nak.end();

  debug("Sending NAK\n");

  // Send the DHCPNAK - Broadcast
  socket_.bcast(server_id_, DHCP_CLIENT_PORT, buffer, sizeof(buffer));
}

bool DHCPD::valid_options(const Message* msg) const {
  const Message_reader reader{msg};
  int num_bytes = 0; // this may be unecessary due to the limit in message reader
  int num_options = reader.parse_options([&num_bytes] (const auto* opt) {
    num_bytes += opt->size();
  });

  return (num_bytes <= Message::LIMIT and num_options <= MAX_NUM_OPTIONS);
}

Record::byte_seq DHCPD::get_client_id(const Message* msg) const {
  const Message_reader reader{msg};

  const auto* opt = reader.find_option<option::client_identifier>();

  // client id if found
  if(opt != nullptr)
    return {opt->val, opt->val + opt->length};
  // else use client hardware address
  else
    return {msg->chaddr.begin(), msg->chaddr.end()};
}

ip4::Addr DHCPD::get_requested_ip_in_opts(const Message* msg) const {
  Message_reader reader{msg};
  const auto* opt = reader.find_option<option::req_addr>();
  return (opt != nullptr) ? *(opt->addr<ip4::Addr>()) : 0;
}

ip4::Addr DHCPD::get_remote_netmask(const Message* msg) const {
  Message_reader reader{msg};
  const auto* opt = reader.find_option<option::subnet_mask>();
  return (opt != nullptr) ? *(opt->addr<ip4::Addr>()) : 0;
}

bool DHCPD::on_correct_network(const Message* msg) const {
  // The client is on the wrong network if the result of applying the local subnet mask or remote subnet
  // mask (if giaddr is not zero) to requested IP address option value doesn't match reality

  const auto subnet = (msg->giaddr == 0) ? netmask_ : get_remote_netmask(msg);

  // Check if remote subnet mask from client exists
  if(UNLIKELY(subnet == 0))
    return false;

  const auto requested_ip = get_requested_ip_in_opts(msg);

  // Check if requested ip is valid
  if(UNLIKELY(requested_ip == 0))
    return false;

  // Check if requested IP is on the same subnet as the server
  if (network_address(requested_ip) == network_address(server_id_))
    return true;

  return false;
}

void DHCPD::clear_offered_ip(ip4::Addr ip) {
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

void DHCPD::print(const Message* msg) const
{
  debug("Printing:\n");

  debug("OP: %u\n", msg->op);
  debug("HTYPE: %u\n", msg->htype);
  debug("HLEN: %u\n", msg->hlen);
  debug("HOPS: %u\n", msg->hops);
  debug("XID: %u\n", msg->xid);
  debug("SECS: %u\n", msg->secs);
  debug("FLAGS: %u\n", msg->flags);
  debug("CIADDR (ip4::Addr): %s\n", msg->ciaddr.to_string().c_str());
  debug("YIADDR (ip4::Addr): %s\n", msg->yiaddr.to_string().c_str());
  debug("SIADDR (ip4::Addr): %s\n", msg->siaddr.to_string().c_str());
  debug("GIADDR (ip4::Addr): %s\n", msg->giaddr.to_string().c_str());

  debug("\nCHADDR:\n");
  for (int i = 0; i < Message::CHADDR_LEN; i++)
    debug("%u ", msg->chaddr[i]);
  debug("\n");

  debug("\nSNAME:\n");
  for (int i = 0; i < Message::SNAME_LEN; i++)
    debug("%u ", msg->sname[i]);
  debug("\n");

  debug("\nFILE:\n");
  for (int i = 0; i < Message::FILE_LEN; i++)
    debug("%u ", msg->file[i]);
  debug("\n");

  debug("\nMAGIC:\n");
  for (int i = 0; i < 4; i++)
    debug("%u ", msg->magic[i]);
  debug("\n");

  // Opts
  const Message_reader reader{msg};
  reader.parse_options([](const auto* opt) {
    debug("\nOptions->code: %d\n", opt->code);
    debug("\nOptions->length: %d\n", opt->length);

    debug("\nOptions->val: ");
    for (size_t i = 0; i < opt->length; i++)
      debug("%d ", opt->val[i]);
    debug("\n");
  });
}
