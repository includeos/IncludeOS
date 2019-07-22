
#pragma once

#include "header.hpp"
#include <net/iana.hpp>
#include <delegate>

namespace net::ip6 {

  struct Extension_header
  {
    uint8_t  next_header;
    uint8_t  hdr_ext_len;
    uint16_t opt_1;
    uint32_t opt_2;

    Protocol proto() const
    {
      return static_cast<Protocol>(next_header);
    }
    uint8_t size() const
    {
      return sizeof(Extension_header) + hdr_ext_len;
    }
    uint8_t extended() const
    {
      return hdr_ext_len;
    }
  } __attribute__((packed));

  /**
   * @brief      Parse the upper layer protocol (TCP/UDP/ICMPv6).
   *             If none, IPv6_NONXT is returned.
   *
   * @param[in]  start  The start of the extension header
   * @param[in]  proto  The protocol
   *
   * @return     Upper layer protocol
   */
  Protocol parse_upper_layer_proto(const uint8_t* start, const uint8_t* end, Protocol proto);

  using Extension_header_inspector = delegate<void(const Extension_header*)>;
  /**
   * @brief      Iterate and inspect all extension headers
   *
   * @param[in]  start       The start of the extension header
   * @param[in]  proto       The protocol
   * @param[in]  on_ext_hdr  Function to be called on every extension header
   *
   * @return     Number of _bytes_ occupied by extension headers (options)
   */
  uint16_t parse_extension_headers(const Extension_header* start, Protocol proto,
                                   Extension_header_inspector on_ext_hdr);

}
