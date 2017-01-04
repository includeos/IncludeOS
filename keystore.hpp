
#ifndef MENDER_KEYSTORE_HPP
#define MENDER_KEYSTORE_HPP

#include "common.hpp"
#include <fs/disk.hpp>
#include <botan/botan.h>
#include <botan/x509cert.h>
#include <botan/rsa.h>
#include <botan/data_src.h>
#include <botan/pkcs8.h>
#include <botan/rng.h>
#include <botan/system_rng.h>
#include <botan/pubkey.h>
#include <botan/hex.h>

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

  Keystore::Keystore(Storage store)
    : store_{nullptr}
  {
    printf("<Keystore> Constructing keystore with a shared disk\n");
    generate();
  }

  Keystore::Keystore(Storage store, std::string kname)
    : store_(store),
      key_name_{std::move(kname)}
  {
    printf("<Keystore> Constructing keystore with a shared disk and keyname\n");
    load();
  }

  Keystore::Keystore(std::string key)
    : store_(nullptr),
    key_name_{"N/A"}
  {
    printf("<Keystore> Constructing keystore with the private key itself\n");
    Botan::DataSource_Memory data{key};
    private_key_.reset(dynamic_cast<Private_key*>(Botan::PKCS8::load_key(data, Botan::system_rng())));
    //assert(private_key_->check_key(Botan::system_rng(), true));
  }

  void Keystore::load()
  {
    printf("<Keystore> Loading private key...\n");
    auto buffer = store_->fs().read_file(key_name_);
    Botan::DataSource_Memory data{buffer.data(), buffer.size()};
    private_key_.reset(dynamic_cast<Private_key*>(Botan::PKCS8::load_key(data, Botan::system_rng())));
    printf("%s\n ... Done.\n", Botan::PKCS8::PEM_encode(*private_key_).c_str());
  }

  void Keystore::generate()
  {
    printf("<Keystore> Generating private key ...\n");
    private_key_ = std::make_unique<Private_key>(Botan::system_rng(), 1024);
    printf("%s\n ... Done.\n", Botan::PKCS8::PEM_encode(*private_key_).c_str());
  }

  Public_PEM Keystore::public_PEM()
  {
    auto pem = Botan::X509::PEM_encode(public_key());
    //printf("PEM:\n%s\n", pem.c_str());
    //auto pem = Botan::PEM_Code::encode(data, "PUBLIC KEY");
    return pem;
  }

  byte_seq Keystore::sign(const byte_seq& data)
  {
    // https://botan.randombit.net/manual/pubkey.html#signatures
    Botan::PK_Signer signer(*private_key_, "EMSA3(SHA-256)"); // EMSA3 for backward compability with PKCS1_15
    auto signature = signer.sign_message((uint8_t*)data.data(), data.size(), Botan::system_rng());
    //printf("Sign:\n%s\n", Botan::hex_encode(signature).c_str());
    return signature;
  }

} // < namespace mender

#endif
