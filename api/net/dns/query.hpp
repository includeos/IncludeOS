
#pragma once

#include "dns.hpp"

namespace net::dns {

  struct Query {
    const id_t        id;
    const std::string hostname;
    const Record_type rtype;

    Query(std::string hostname, const Record_type rtype)
      : Query{generate_id(), std::move(hostname), rtype}
    {}

    Query(const id_t id, std::string hostname, const Record_type rtype)
      : id{id},
        hostname{std::move(hostname)},
        rtype{rtype}
    {}

    size_t write(char* buffer) const;

  private:
    // TODO: for now, needs to be randomized
    static unsigned short generate_id()
    {
      static unsigned short id = 0;
      return ++id;
    }

    int write_formatted_hostname(char* qname) const
    { return dns::encode_name(this->hostname, qname); }
  };

}
