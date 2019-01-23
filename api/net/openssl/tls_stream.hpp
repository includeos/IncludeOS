#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <net/stream_buffer.hpp>

//#define VERBOSE_OPENSSL 0
#ifdef VERBOSE_OPENSSL
#define TLS_PRINT(fmt, ...) printf("TLS_Stream");printf(fmt, ##__VA_ARGS__)
#else
#define TLS_PRINT(fmt, ...) /* fmt */
#endif

namespace openssl
{
  struct TLS_stream : public net::StreamBuffer
  {
    using Stream_ptr = net::Stream_ptr;

    TLS_stream(SSL_CTX* ctx, Stream_ptr, bool outgoing = false);
    TLS_stream(Stream_ptr, SSL* ctx, BIO*, BIO*);
    virtual ~TLS_stream();

    void write(buffer_t buffer) override;
    void write(const std::string&) override;
    void write(const void* buf, size_t n) override;
    void close() override;

    net::Socket local() const override {
      return m_transport->local();
    }
    net::Socket remote() const override {
      return m_transport->remote();
    }
    std::string to_string() const override {
      return m_transport->to_string();
    }

    bool is_connected() const noexcept override {
      return handshake_completed() && m_transport->is_connected();
    }
    bool is_writable() const noexcept override {
      return (not write_congested()) && is_connected() && m_transport->is_writable();
    }
    bool is_readable() const noexcept override {
      return m_transport->is_readable();
    }
    bool is_closing() const noexcept override {
      return m_transport->is_closing();
    }
    bool is_closed() const noexcept override {
      return m_transport->is_closed();
    }

    int get_cpuid() const noexcept override {
      return m_transport->get_cpuid();
    }

    Stream* transport() noexcept override {
      return m_transport.get();
    }

    void handle_read_congestion() override;
    void handle_write_congestion() override;

  private:
    void handle_data();
    int  decrypt(const void *data,int size);
    int  send_decrypted();
    bool tls_read(buffer_t);
    int  tls_perform_stream_write();
    int  tls_perform_handshake();
    bool handshake_completed() const noexcept;
    void close_callback_once();

    enum status_t {
      STATUS_OK,
      STATUS_WANT_IO,
      STATUS_FAIL
    };
    status_t status(int n) const noexcept;
    Stream_ptr m_transport = nullptr;
    SSL*  m_ssl    = nullptr;
    BIO*  m_bio_rd = nullptr;
    BIO*  m_bio_wr = nullptr;
    bool  m_busy = false;
    bool  m_deferred_close = false;
  };

} // openssl
