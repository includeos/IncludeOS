
#ifndef MENDER_AUTH_HPP
#define MENDER_AUTH_HPP

#include "common.hpp"
#include "json.hpp"

namespace mender {

  /** Device identifier. e.g. JSON(MAC_addr) */
  using Dev_id      = std::string;
  /** Public key */
  using Public_PEM  = std::string;
  /** Token */
  using Auth_token  = byte_seq;

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
    nlohmann::json j;
    j["tenant_token"] = std::string{tenant_token.begin(), tenant_token.end()};
    j["seq_no"]       = seq_no;
    j["id_data"]      = id_data;
    j["pubkey"]       = pubkey;

    auto str = j.dump();
    //printf("%s\n", str.c_str());
    return {str.begin(), str.end()};
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
