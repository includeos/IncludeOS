
#ifndef MENDER_KEYSTORE_HPP
#define MENDER_KEYSTORE_HPP

#include "common.hpp"

// placeholder for botan
namespace x509 {
  using byte_seq = mender::byte_seq;

  byte_seq PEM_encode(mender::Public_key&)
  {
    return "";
  }
}

// placeholder for botan
namespace PEM_Code {
  using byte_seq = mender::byte_seq;

  byte_seq encode(byte_seq data, const std::string& label)
  {
    return label;
  }
}

namespace mender {

  using Public_key  = std::string; // RSA_PublicKey
  using Private_key = std::string; // RSA_PrivateKey
  using Public_PEM  = std::string;

  class Keystore {
  public:
    static constexpr size_t KEY_LENGTH = 2048;
    using Store = std::string; // Memdisk to read from?

  public:
    Keystore(Store, std::string name);

    byte_seq generate_signature(const byte_seq& data);

    Public_PEM public_PEM();

    Public_key& public_key()
    { return private_key_; }

    void load();

    void generate();

  private:
    Store store_;
    Private_key private_key_;
    std::string key_name_;
  };

  Keystore::Keystore(Store store, std::string kname)
    : store_(store),
      key_name_(kname)
  {
    load();
  }

  void Keystore::load()
  {
    // read Private_key from Store somehow...
  }

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
    auto data = x509::PEM_encode(public_key());
    auto pem = PEM_Code::encode(data, "PUBLIC_KEY");
    return pem;
  }

  byte_seq Keystore::generate_signature(const byte_seq& data)
  {
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
