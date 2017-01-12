
#include "auth_manager.hpp"

namespace mender {

  Auth_manager::Auth_manager(std::unique_ptr<Keystore> ks, Dev_id id, uint64_t seq)
    : keystore_(std::move(ks)),
      id_data_(std::move(id)), seq_(seq)
  {
    std::string tkn{""};
    tenant_token_ = Auth_token{tkn.begin(), tkn.end()};
  }

  Auth_request Auth_manager::make_auth_request()
  {
    Request_data authd;

    // Populate request data
    authd.id_data       = this->id_data_;
    authd.pubkey        = this->keystore_->public_PEM();
    authd.tenant_token  = this->tenant_token_;
    authd.seq_no        = this->seq_++;

    printf("<Auth_manager> Next seqno=%lli\n", seq_);
    // Get serialized bytes
    const auto reqdata = authd.serialized_bytes();

    // Generate signature from payload
    const auto signature = keystore_->sign(reqdata);

    return Request {
      .data       = reqdata,
      .token      = this->tenant_token_,
      .signature  = signature
    };
  }

}

