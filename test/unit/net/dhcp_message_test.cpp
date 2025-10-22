// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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

#include <common.cxx>
#include <net/dhcp/message.hpp>
#include <hw/mac_addr.hpp>

MAC::Addr test_mac()
{ return {0xBA,0xDB,0xEE,0xFB,0x00,0xB5}; }

struct test_opt
{
  uint8_t code;
  uint8_t length;
  uint8_t val[0];
};

// Creates a DHCP DISCOVERY message on the buffer
net::dhcp::Message* create_discovery_msg(uint8_t* buffer)
{
  using namespace net;
  using namespace net::dhcp;
  auto* msg = reinterpret_cast<Message*>(buffer);

  msg->op     = static_cast<uint8_t>(op_code::BOOTREQUEST);
  msg->htype  = static_cast<uint8_t>(htype::ETHER);
  msg->hlen   = ETH_ALEN;
  msg->hops   = 0;
  msg->xid    = htonl(322420);
  msg->secs   = 0;
  msg->flags  = htons(static_cast<uint16_t>(flag::BOOTP_BROADCAST));
  msg->ciaddr = 0;
  msg->yiaddr = 0;
  msg->siaddr = 0;
  msg->giaddr = 0;

  MAC::Addr link_addr = test_mac();
  memset(msg->chaddr.data(), 0, Message::CHADDR_LEN);
  memcpy(msg->chaddr.data(), &link_addr, ETH_ALEN);

  memset(msg->sname.data(), 0, Message::SNAME_LEN + Message::FILE_LEN);

  msg->magic[0] =  99;
  msg->magic[1] = 130;
  msg->magic[2] =  83;
  msg->magic[3] =  99;

  auto* raw = reinterpret_cast<uint8_t*>(&msg->options[0]);
  auto* opt = reinterpret_cast<test_opt*>(raw);

  // DHCP discover
  opt->code   = option::DHCP_MESSAGE_TYPE;
  opt->length = 1;
  opt->val[0] = static_cast<uint8_t>(message_type::DISCOVER);
  raw += 3;

  // DHCP client identifier
  opt = reinterpret_cast<test_opt*>(raw);
  opt->code   = option::DHCP_CLIENT_IDENTIFIER;
  opt->length = 7;
  opt->val[0] = static_cast<uint8_t>(htype::ETHER);
  memcpy(&opt->val[1], &link_addr, ETH_ALEN);
  raw += 9;

  // DHCP Parameter Request Field
  opt = reinterpret_cast<test_opt*>(raw);
  opt->code   = option::DHCP_PARAMETER_REQUEST_LIST;
  opt->length = 3;
  opt->val[0] = option::ROUTERS;
  opt->val[1] = option::SUBNET_MASK;
  opt->val[2] = option::DOMAIN_NAME_SERVERS;
  raw += 5;

  // END
  opt = reinterpret_cast<test_opt*>(raw);
  opt->code   = option::END;
  opt->length = 0;

  return msg;
}

CASE("Reading Message and parsing options with Message_view")
{
  using namespace net::dhcp;
  uint8_t buffer[Message::size()];

  const auto* msg = create_discovery_msg(&buffer[0]);

  const Message_reader view{msg};
  MAC::Addr link_addr = test_mac();

  EXPECT(view.xid() == 322420);

  EXPECT(view.ciaddr() == 0);
  EXPECT(view.yiaddr() == 0);
  EXPECT(view.siaddr() == 0);
  EXPECT(view.giaddr() == 0);

  EXPECT(view.chaddr<MAC::Addr>() == link_addr);

  auto* anon_opt = view.find_option(option::DHCP_MESSAGE_TYPE);

  EXPECT(anon_opt != nullptr);
  EXPECT(anon_opt->code != option::END);
  EXPECT(anon_opt->code == option::DHCP_MESSAGE_TYPE);
  EXPECT(anon_opt->val[0] == static_cast<uint8_t>(message_type::DISCOVER));

  auto* message_opt = view.find_option<option::message_type>();

  EXPECT(message_opt != nullptr);
  EXPECT(message_opt->code == option::DHCP_MESSAGE_TYPE);
  EXPECT(message_opt->type() == message_type::DISCOVER);
  EXPECT(anon_opt == message_opt);

  anon_opt = view.find_option(option::DHCP_CLIENT_IDENTIFIER);

  EXPECT(anon_opt != nullptr);
  EXPECT(anon_opt->code != option::END);
  EXPECT(anon_opt->code == option::DHCP_CLIENT_IDENTIFIER);
  EXPECT(anon_opt->length == 7);
  EXPECT(anon_opt->val[0] == static_cast<uint8_t>(htype::ETHER));
  EXPECT(std::memcmp(&anon_opt->val[1], &link_addr, ETH_ALEN) == 0);

  auto* client_id_opt = view.find_option<option::client_identifier>();

  EXPECT(client_id_opt != nullptr);
  EXPECT(client_id_opt->code == option::DHCP_CLIENT_IDENTIFIER);
  EXPECT(*client_id_opt->addr<MAC::Addr>() == link_addr);
  EXPECT(anon_opt == client_id_opt);

  anon_opt = view.find_option(option::DHCP_PARAMETER_REQUEST_LIST);

  EXPECT(anon_opt != nullptr);
  EXPECT(anon_opt->code != option::END);
  EXPECT(anon_opt->code == option::DHCP_PARAMETER_REQUEST_LIST);
  EXPECT(anon_opt->length == 3);
  EXPECT(anon_opt->val[0] == option::ROUTERS);
  EXPECT(anon_opt->val[1] == option::SUBNET_MASK);
  EXPECT(anon_opt->val[2] == option::DOMAIN_NAME_SERVERS);

  auto* param_req_list_opt = view.find_option<option::param_req_list>();

  EXPECT(param_req_list_opt != nullptr);
  EXPECT(param_req_list_opt->code == option::DHCP_PARAMETER_REQUEST_LIST);
  EXPECT(anon_opt == param_req_list_opt);

  int i = 0;
  auto y = view.parse_options([&i] (const option::base* opt)
  {
    ++i;
  });
  EXPECT(i == 3);
  EXPECT(y == i);
}

CASE("Creating Message and adding options with Message_view")
{
  using namespace net::dhcp;
  uint8_t buffer[Message::size()];
  std::memset(buffer, 0, Message::size());

  Message_writer view{&buffer[0], op_code::BOOTREQUEST, message_type::DISCOVER};

  auto* message_opt = view.find_option<option::message_type>();

  EXPECT(message_opt != nullptr);
  EXPECT(message_opt->code == option::DHCP_MESSAGE_TYPE);
  EXPECT(message_opt->type() == message_type::DISCOVER);

  MAC::Addr link_addr = test_mac();

  view.set_hw_addr(htype::ETHER, sizeof(MAC::Addr));

  view.set_xid(322420);

  EXPECT(view.xid() == 322420);

  view.set_flag(flag::BOOTP_BROADCAST);

  view.set_chaddr(&link_addr);

  EXPECT(view.chaddr<MAC::Addr>() == link_addr);

  view.add_option<option::client_identifier>(htype::ETHER, &link_addr);

  auto* client_id_opt = view.find_option<option::client_identifier>();

  EXPECT(client_id_opt != nullptr);
  EXPECT(client_id_opt->code == option::DHCP_CLIENT_IDENTIFIER);
  EXPECT(*client_id_opt->addr<MAC::Addr>() == link_addr);

  view.add_option<option::param_req_list>(std::vector<option::Code>{
    option::ROUTERS,
    option::SUBNET_MASK,
    option::DOMAIN_NAME_SERVERS
  });

  view.end();

  uint8_t buffer2[Message::size()];
  std::memset(buffer2, 0, Message::size());

  create_discovery_msg(&buffer2[0]);

  EXPECT(std::memcmp(&buffer[0], &buffer2[0], Message::size()) == 0);
}
