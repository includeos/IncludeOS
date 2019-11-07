#pragma once

#include <net/ip6/addr.hpp>
#include <chrono>

namespace net::mld {

  // MLDv1 Message
  struct Message
  {
    uint16_t  max_res_delay;
    uint16_t  reserved{0x0};
    ip6::Addr mcast_addr;

    Message(uint16_t delay, ip6::Addr mcast)
      : max_res_delay{htons(delay)},
        mcast_addr{std::move(mcast)}
    {}
  };

  struct Query : public Message
  {
    Query(uint16_t delay)
      : Message{delay, ip6::Addr::addr_any}
    {}

    bool is_general() const noexcept
    { return this->mcast_addr == ip6::Addr::addr_any; }

    auto max_res_delay_ms() const noexcept
    { return std::chrono::milliseconds{ntohs(max_res_delay)}; }
  };

  struct Report : public Message
  {
    Report(ip6::Addr mcast)
      : Message{0, std::move(mcast)}
    {}
  };

// MLDv2
namespace v2 {

  struct Query
  {
    uint16_t  max_res_code;
    uint16_t  reserved{0x0};
    ip6::Addr mcast_addr;
    uint8_t   flags;
    /*uint8_t   reserved : 4,
              supress  : 1,
              qrc      : 3;*/
    uint8_t   qqic;
    uint16_t  num_srcs;
    ip6::Addr sources[0];

  };

  enum Record_type : uint8_t
  {
    IS_INCLUDE = 1,
    IS_EXCLUDE = 2,
    CHANGE_TO_INCLUDE = 3,
    CHANGE_TO_EXCLUDE = 4,
    ALLOW_NEW_SOURCES = 5,
    BLOCK_OLD_SOURCES = 6
  };

  struct Mcast_addr_record
  {
    uint8_t   rec_type;
    uint8_t   data_len{0};
    uint16_t  num_src{0};
    ip6::Addr multicast;
    ip6::Addr sources[0];

    Mcast_addr_record(Record_type rec, ip6::Addr mcast)
      : rec_type{rec},
        multicast{std::move(mcast)}
    {}

    size_t size() const noexcept
    { return sizeof(Mcast_addr_record) + num_src * (sizeof(ip6::Addr) + data_len); }
  };

  struct Report
  {
    uint16_t  reserved{0x0};
    uint16_t  num_records{0};
    char      records[0];

    uint16_t insert(uint16_t offset, Mcast_addr_record&& rec)
    {
      auto* data = records + offset;

      std::memcpy(data, &rec, rec.size());

      num_records = ntohs(num_records + 1);

      return rec.size();
   }

  };

}

}
