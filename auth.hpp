
#ifndef MENDER_AUTH_HPP
#define MENDER_AUTH_HPP

#include "common.hpp"

namespace mender {

  /** Device identifier. e.g. JSON(MAC_addr) */
  using Dev_id      = std::string;
  /** Public key */
  using Public_PEM  = std::string;
  /** Token */
  using Auth_token  = std::string;

  using Writer      = std::string; // rapidjson::Writer


  /** Authorization request data, built by the caller (Auth_manager) */
  struct Auth_request_data {
    Dev_id      id_data;
    Auth_token  tenant_token;
    Public_PEM  pubkey;
    int64_t     seq_no;

    inline byte_seq serialized_bytes() const;

  private:
    template <typename Writer>
    void serialize(Writer& wr) const;
  };

  inline byte_seq Auth_request_data::serialized_bytes() const
  {
    Writer wr;
    serialize(wr);
    //return wr.ToString();
    return wr;
  }

  template <typename Writer>
  void Auth_request_data::serialize(Writer& wr) const
  {

  }

  /** An authorization request */
  struct Auth_request {
    byte_seq   data;
    Auth_token token;
    byte_seq   signature;
  };

} // < namespace mender

#endif
