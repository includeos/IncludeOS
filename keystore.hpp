
#ifndef MENDER_KEYSTORE_HPP
#define MENDER_KEYSTORE_HPP

#include "common.hpp"
#include <fs/disk.hpp>
#include <botan/botan.h>
#include <botan/x509cert.h>
#include <botan/rsa.h>
#include <botan/data_src.h>

namespace mender {

  using Public_key  = Botan::RSA_PublicKey; // RSA_PublicKey
  using Private_key = Botan::RSA_PrivateKey; // RSA_PrivateKey
  using Public_PEM  = std::string;

  class Keystore {
  public:
    static constexpr size_t KEY_LENGTH = 2048;
    using Store = std::shared_ptr<fs::Disk>; // Memdisk to read from?

  public:
    Keystore(Store, std::string name);
    Keystore(std::string key);

    byte_seq sign(const byte_seq& data);

    Public_PEM public_PEM();

    const Botan::Public_Key& public_key()
    { return *private_key_; }

    void load();

    void generate();

  private:
    Store store_;
    Botan::Public_Key* private_key_;
    std::string key_name_;

    Public_key* load_from_PEM(fs::Buffer&) const;

  };

  Keystore::Keystore(Store store, std::string kname)
    : store_(store),
      key_name_{std::move(kname)}
  {
    load();
  }

  Keystore::Keystore(std::string key)
    : store_(nullptr),
    key_name_{"N/A"}
  {
    Botan::DataSource_Memory data{key};
    private_key_ = Botan::X509::load_key(data);
  }

  void Keystore::load()
  {
    auto buffer = store_->fs().read_file("/"+key_name_);
    Botan::DataSource_Memory data{buffer.data(), buffer.size()};
    private_key_ = Botan::X509::load_key(data);
  }

  /*Public_key* Keystore::load_from_PEM(fs::Buffer& buffer) const
  {
    Botan::DataSource_Memory data{buffer.data(), buffer.size()};
    //auto block = Botan::PEM_Code::decode({(char*)buffer.data(), buffer.size()}, "PRIVATE KEY");

    //printf("%s\n", block.c_str());
    auto* key = Botan::X509::load_key(data);

    return key;
  }*/

  void Keystore::generate()
  {
    /*
      key, err := rsa.GenerateKey(rand.Reader, RsaKeyLength)
      if err != nil {
        return err
      }

      k.private = key

      return nil
    */
  }

  Public_PEM Keystore::public_PEM()
  {
    /*
      data, err := x509.MarshalPKIXPublicKey(k.Public())
      if err != nil {
        return "", errors.Wrapf(err, "failed to marshal public key")
      }

      buf := &bytes.Buffer{}
      err = pem.Encode(buf, &pem.Block{
        Type:  "PUBLIC KEY", // PKCS1
        Bytes: data,
      })
      if err != nil {
        return "", errors.Wrapf(err, "failed to encode public key to PEM")
      }

      return buf.String(), nil
    */
    auto pem = Botan::X509::PEM_encode(public_key());
    //auto pem = Botan::PEM_Code::encode(data, "PUBLIC KEY");
    return pem;
  }

  byte_seq Keystore::sign(const byte_seq& data)
  {
    //private_key_.create_signature_op(rand, data, "SHA-256");
    /*
      hash := crypto.SHA256
      h := hash.New()
      h.Write(data)
      sum := h.Sum(nil)

      return rsa.SignPKCS1v15(rand.Reader, k.private, hash, sum)
    */
    return "";
  }

} // < namespace mender

#endif
