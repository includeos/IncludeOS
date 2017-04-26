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

#pragma once
#ifndef NET_DHCP_MESSAGE_HPP
#define NET_DHCP_MESSAGE_HPP

#include "dhcp4.hpp"
#include "options.hpp"
#include <net/ip4/addr.hpp>
#include <delegate> // option inspection

namespace net {
namespace dhcp {

/**
 * @brief      A DHCP message as described in RFC 2131
 */
struct Message
{
  using Addr = ip4::Addr;
  static const uint8_t CHADDR_LEN =  16;
  static const uint8_t SNAME_LEN  =  64;
  static const uint8_t FILE_LEN   = 128;
  static constexpr uint16_t LIMIT{DHCP_VEND_LEN}; // 304
  using Chaddr_arr = std::array<uint8_t, CHADDR_LEN>;
  using Sname_arr  = std::array<uint8_t, SNAME_LEN>;
  using File_arr   = std::array<uint8_t, FILE_LEN>;
  using Magic_arr  = std::array<uint8_t, 4>;


  uint8_t   op;           // message opcode
  uint8_t   htype;        // hardware addr type
  uint8_t   hlen;         // hardware addr length
  uint8_t   hops;         // relay agent hops from client
  uint32_t  xid;          // transaction ID
  uint16_t  secs;         // seconds since start
  uint16_t  flags;        // flag bits
  Addr      ciaddr;       // client IP address
  Addr      yiaddr;       // client IP address
  Addr      siaddr;       // IP address of next server
  Addr      giaddr;       // DHCP relay agent IP address
  Chaddr_arr chaddr;  // client hardware address
  Sname_arr  sname;   // server name
  File_arr   file;    // BOOT filename
  Magic_arr  magic;   // option_format aka magic
  option::base          options[0];

  /**
   * @brief      Adds an option to the message.
   *
   * @param[in]  offset     The offset from where options begins (where to be placed)
   * @param[in]  <unnamed>  Constructor arguments for Opt
   *
   * @tparam     Opt        The option to add
   * @tparam     Args       The arguments for constructing the option.
   *
   * @return     Returns a reference to the newly created option.
   */
  template <typename Opt, typename... Args>
  Opt& add_option(const uint16_t offset, Args&&... args)
  {
    static_assert(std::is_base_of<option::base, Opt>::value, "Option do not inherit option::base");
    static_assert(sizeof(Opt) == sizeof(option::base), "Option do not have the same storage as option::base");
    return *(new (reinterpret_cast<uint8_t*>(&options[0]) + offset) Opt(args...));
  }

  /**
   * @brief      Returns some kind of standard size for a message.
   *             Useful for creating buffers that are big enough.
   *
   * @return     Returns some kind fo standard size for a message.
   */
  static constexpr uint16_t size() noexcept
  { return sizeof(Message) + LIMIT; }
}; // < struct Message

/**
 * @brief      A readable DHCP message. Needs to be inherited.
 *             This class is used to determine if the storage should be const or not.
 *
 * @tparam     Storage  Message or const Message
 */
template <typename Storage>
class Readable_message {
public:

  op_code op() const noexcept
  { return static_cast<op_code>(message_.op); }

  auto ciaddr() const noexcept
  { return message_.ciaddr; }

  auto yiaddr() const noexcept
  { return message_.yiaddr; }

  auto siaddr() const noexcept
  { return message_.siaddr; }

  auto giaddr() const noexcept
  { return message_.giaddr; }

  uint32_t xid() const noexcept
  { return ntohl(message_.xid); }

  /**
   * @brief      Returns the client hardware address.
   *
   * @tparam     Addr  The type of the hardware address.
   *
   * @return     The client hardware address.
   */
  template <typename Addr>
  const Addr& chaddr() const
  { return *reinterpret_cast<const Addr*>(&message_.chaddr[0]); }

    /**
   * @brief      Find the option of the given type.
   *
   * @tparam     Opt   The option to be found.
   *
   * @return     A pointer to the option if found, nullptr if not.
   */
  template <typename Opt>
  const Opt* find_option() const
  {
    const auto* opt = find_option(Opt::CODE);

    return (opt->code == Opt::CODE)
      ? static_cast<const Opt*>(opt) : nullptr;
  }

  /**
   * @brief      Find an "anonymous" option with the given code.
   *
   * @param[in]  code  The code to look for.
   *
   * @return     A pointer to an "anonymous" option, most likely with code == END if not found.
   */
  const option::base* find_option(option::Code code) const
  {
    auto* raw = reinterpret_cast<uint8_t*>(const_cast<option::base*>(&message_.options[0]));
    auto* opt = reinterpret_cast<option::base*>(raw);

    while (opt->code != code && opt->code != option::END && opt < max_opt_addr())
    {
      raw += opt->size();
      opt = reinterpret_cast<option::base*>(raw);
    }
    return opt;
  }

  /** Invoked with a const pointer of an "anonymous" option. */
  using Option_inspector = delegate<void(const option::base*)>;
  /**
   * @brief      Iterates over all the options, calling the on_option
   *             for every hit.
   *
   * @param[in]  on_option  On option inspector to be called for every hit.
   *
   * @return     Returns the total number of options found.
   */
  uint8_t parse_options(Option_inspector on_option) const
  {
    Expects(on_option);
    uint8_t x = 0;

    auto* raw = reinterpret_cast<uint8_t*>(const_cast<option::base*>(&message_.options[0]));
    auto* opt = reinterpret_cast<option::base*>(raw);

    while(opt->code != option::END && opt < max_opt_addr())
    {
      if(opt->code != option::PAD)
      {
        ++x;
        on_option(opt);
      }
      raw += opt->size();
      opt = reinterpret_cast<option::base*>(raw);
    }

    return x;
  }

  /**
   * @brief      Returns the maximum address for an option
   *             based on the limit set in Message.
   *             Used for restrict iteration on options.
   *
   * @return     The maximum address for an option.
   */
  const option::base* max_opt_addr() const noexcept
  { return &message_.options[0] + (Message::LIMIT / sizeof(option::base)); }

protected:
  Storage& message_;

  Readable_message(Storage& msg)
    : message_{msg}
  {}

}; // < class Readable_message

/**
 * @brief      Helper for reading a message and options.
 */
class Message_reader : public Readable_message<const Message> {
public:
  /**
   * @brief      Constructs a Message_reader over a message.
   *             Used when reading existing messages.
   *
   * @param[in]  msg   The message
   */
  explicit Message_reader(const Message* msg) noexcept
    : Readable_message{*msg}
  {
    Expects(msg != nullptr);
  }

  /**
   * @brief      Constructs a Message_reader over a buffer.
   *
   * @param[in]  buffer  The buffer
   */
  explicit Message_reader(const uint8_t* buffer) noexcept
    : Message_reader{reinterpret_cast<const Message*>(buffer)}
  {}

}; // < class Message_reader

/**
 * @brief      Helper for creating new messages and adding options.
 */
class Message_writer : public Readable_message<Message> {
public:
  using Addr = Message::Addr;

  /**
   * @brief      Constructs a new Message_view over a buffer.
   *             Used when creating new messages.
   *             The user needs to make sure the buffer is big enough.
   *
   * @param      msg   The message
   * @param[in]  op    The op code
   * @param[in]  type  The DHCP message type
   */
  explicit Message_writer(Message* msg, const op_code op, const message_type type) noexcept
    : Readable_message{*msg}
  {
    Expects(msg != nullptr);
    // null the buffer
    reset();
    set_op(op);
    // add magic cookie
    set_magic();
    // add message option
    add_option<option::message_type>(type);
  }

  /**
   * @brief      Constructs a new Message_view over a buffer.
   *             Used when creating new messages.
   *             The user needs to make sure the buffer is big enough.
   *
   * @param      buffer   The buffer
   * @param[in]  op       The op code
   * @param[in]  type     The DHCP message type
   */
  explicit Message_writer(uint8_t* buffer, const op_code op, const message_type type) noexcept
    : Message_writer{reinterpret_cast<Message*>(buffer), op, type}
  {}

  void set_op(op_code op) noexcept
  { message_.op = static_cast<uint8_t>(op); }

  /**
   * @brief      Sets the hardware address info (type and length)
   *
   * @param[in]  type  The type
   * @param[in]  len   The length
   */
  void set_hw_addr(htype type, uint8_t len) noexcept
  {
    message_.htype = static_cast<uint8_t>(type);
    message_.hlen  = len;
  }

  void set_ciaddr(Addr addr) noexcept
  { message_.ciaddr = addr; }

  void set_yiaddr(Addr addr) noexcept
  { message_.yiaddr = addr; }

  void set_siaddr(Addr addr) noexcept
  { message_.siaddr = addr; }

  void set_giaddr(Addr addr) noexcept
  { message_.giaddr = addr; }

  void set_xid(uint32_t xid) noexcept
  { message_.xid = htonl(xid); }

  void set_flag(flag fl) noexcept
  { message_.flags = htons(static_cast<uint16_t>(fl)); }

  void set_flags(uint16_t fl) noexcept
  { message_.flags = fl; }

  /**
   * @brief      Sets the client hardware address.
   *             htype and hlen need to already be set (set_hw_addr)
   *
   * @param[in]  hwaddr  The hardware address
   *
   * @tparam     Addr    The type of the hardware address.
   */
  template <typename Addr>
  void set_chaddr(const Addr* hwaddr)
  {
    Expects(message_.htype != 0 && "hardware address type must be set");
    Expects(sizeof(Addr) == message_.hlen && "size of Addr is not equal the set length of the hw address");
    // clean just to be sure
    std::memset(message_.chaddr.begin(), 0, Message::CHADDR_LEN);
    std::memcpy(message_.chaddr.begin(), hwaddr, sizeof(Addr));
  }

  void set_sname(const Message::Sname_arr& sname)
  { message_.sname = sname; }

  void set_file(const Message::File_arr& file)
  { message_.file = file; }

  /**
   * @brief      Sets the magic cookie.
   */
  void set_magic() noexcept
  { message_.magic = {{99, 130, 83, 99}}; }

  /**
   * @brief      Returns the option offset, i.e. how far into options we've written.
   *             This is currently only useful when creating messages.
   *
   * @return     The option offset in bytes
   */
  uint16_t option_offset() const noexcept
  { return opt_offset; }

  /**
   * @brief      Resets everything inside the message up to the last option.
   *             See option_offset()
   */
  void reset()
  { memset(&message_, 0, sizeof(Message) + opt_offset); }

  /**
   * @brief      End the message by adding a END option at the end.
   */
  void end()
  { add_option<option::end>(); }

  /**
   * @brief      Adds an option.
   *
   * @param[in]  <unnamed>  Constructor arguments to the Opt.
   *
   * @tparam     Opt        The option
   * @tparam     Args       The args to construct the option
   *
   * @return     A reference to the newly created option.
   */
  template <typename Opt, typename... Args>
  Opt& add_option(Args&&... args) noexcept
  {
    Expects((opt_offset + sizeof(option::base)) < Message::LIMIT &&
      "Adding option past artifical limit set on message (review the limit in dhcp::Message::size())");

    auto& opt = message_.add_option<Opt>(opt_offset, std::forward<Args>(args)...);
    // increase the option offset by the size of the newly created option
    opt_offset += opt.size();
    return opt;
  }

private:
  uint16_t opt_offset{0};

}; // < class Message_writer

} // < namespace dhcp
} // < namespace net

#endif
