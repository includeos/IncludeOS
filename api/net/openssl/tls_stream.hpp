#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <net/stream.hpp>

//#define VERBOSE_OPENSSL
#ifdef VERBOSE_OPENSSL
#define TLS_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define TLS_PRINT(fmt, ...) /* fmt */
#endif

namespace openssl
{
  struct TLS_stream : public net::Stream
  {
    using Stream_ptr = net::Stream_ptr;

    TLS_stream(SSL_CTX* ctx, Stream_ptr, bool outgoing = false);
    TLS_stream(Stream_ptr, SSL* ctx, BIO*, BIO*);
    virtual ~TLS_stream();

    void write(buffer_t buffer) override;
    void write(const std::string&) override;
    void write(const void* buf, size_t n) override;
    void close() override;
    void reset_callbacks() override;

    net::Socket local() const override {
      return m_transport->local();
    }
    net::Socket remote() const override {
      return m_transport->remote();
    }
    std::string to_string() const override {
      return m_transport->to_string();
    }

    void on_connect(ConnectCallback cb) override {
      m_on_connect = std::move(cb);
    }
    void on_read(size_t, ReadCallback cb) override {
      m_on_read = std::move(cb);
    }
    void on_data(DataCallback cb) override {
      m_on_data = std::move(cb);
    }
    size_t next_size() override {
      // FIXME: implement buffering for read_next
      return 0;
    }
    buffer_t read_next() override {
      // FIXME: implement buffering for read_next
      return{};
    }
    void on_close(CloseCallback cb) override {
      m_on_close = std::move(cb);
    }
    void on_write(WriteCallback cb) override {
      m_on_write = std::move(cb);
    }

    bool is_connected() const noexcept override {
      return handshake_completed() && m_transport->is_connected();
    }
    bool is_writable() const noexcept override {
      return is_connected() && m_transport->is_writable();
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

    size_t serialize_to(void*) const override;

  private:
    void tls_read(buffer_t);
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
    ConnectCallback  m_on_connect = nullptr;
    ReadCallback     m_on_read    = nullptr;
    DataCallback     m_on_data    = nullptr;
    WriteCallback    m_on_write   = nullptr;
    CloseCallback    m_on_close   = nullptr;
  };

} // openssl
