
#ifndef MENDER_KEYSTORE_HPP
#define MENDER_KEYSTORE_HPP

#include "common.hpp"
#include <fs/disk.hpp>
#include <botan/rsa.h> // RSA_PublicKey, RSA_PrivateKey

namespace mender {

  using Public_key  = Botan::RSA_PublicKey; // RSA_PublicKey
  using Private_key = Botan::RSA_PrivateKey; // RSA_PrivateKey
  using Public_PEM  = std::string;
  using Auth_token  = byte_seq;

  class Keystore {
  public:
    static constexpr size_t KEY_LENGTH = 2048;
    using Storage = std::shared_ptr<fs::Disk>; // Memdisk to read from?

  public:
    Keystore(Storage store = nullptr);
    Keystore(Storage, std::string name);
    Keystore(std::string key);

    byte_seq sign(const byte_seq& data);

    Public_PEM public_PEM();

    const Public_key& public_key()
    { return *private_key_; }

    void load();

    void generate();

  private:
    Storage store_;
    std::unique_ptr<Private_key> private_key_;
    std::string key_name_;

    Public_key* load_from_PEM(fs::Buffer&) const;

  };



} // < namespace mender

#endif
