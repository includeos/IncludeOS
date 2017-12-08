#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <net/inet4>

namespace openssl
{
  struct TLS_stream : public net::Stream
  {
    using Stream_ptr = net::Stream_ptr;

    TLS_stream(SSL_CTX* ctx, Stream_ptr t);
    virtual ~TLS_stream();

    void write(buffer_t buffer) override;
    void write(const std::string&) override;
    void write(const void* buf, size_t n) override;
    void close() override;
    void abort() override;
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
    void on_read(size_t n, ReadCallback cb) override {
      m_on_read = std::move(cb);
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

  private:
    void tls_read(buffer_t);
    int  tls_perform_stream_write();
    void close_callback_once();
    bool handshake_completed() const noexcept;

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
    ConnectCallback  m_on_connect = nullptr;
    ReadCallback     m_on_read    = nullptr;
    WriteCallback    m_on_write   = nullptr;
    CloseCallback    m_on_close   = nullptr;
  };

  inline TLS_stream::TLS_stream(SSL_CTX* ctx, Stream_ptr t)
    : m_transport(std::move(t))
  {
    this->m_bio_rd = BIO_new(BIO_s_mem());
    this->m_bio_wr = BIO_new(BIO_s_mem());
    assert(ERR_get_error() == 0 && "Initializing BIOs");
    this->m_ssl = SSL_new(ctx);
    assert(this->m_ssl != nullptr);
    assert(ERR_get_error() == 0 && "Initializing SSL");
    //extern void openssl_setup_rng();
    //openssl_setup_rng();
    // TLS server-mode
    SSL_set_accept_state(this->m_ssl);
    SSL_set_bio(this->m_ssl, this->m_bio_rd, this->m_bio_wr);
    // always-on callbacks
    m_transport->on_read(8192, {this, &TLS_stream::tls_read});
    m_transport->on_close({this, &TLS_stream::close_callback_once});
  }
  inline TLS_stream::~TLS_stream()
  {
    BIO_free(this->m_bio_rd);
    BIO_free(this->m_bio_wr);
    SSL_free(this->m_ssl);
  }

  inline void TLS_stream::write(buffer_t buffer)
  {
    assert(this->is_connected());

    int n = SSL_write(this->m_ssl, buffer->data(), buffer->size());
    auto status = this->status(n);
    if (status == STATUS_FAIL) {
      this->close();
      return;
    }

    do {
      n = tls_perform_stream_write();
    } while (n > 0);
  }
  inline void TLS_stream::write(const std::string& str)
  {
    write(net::tcp::construct_buffer(str.data(), str.data() + str.size()));
  }
  inline void TLS_stream::write(const void* data, const size_t len)
  {
    auto* buf = static_cast<const uint8_t*> (data);
    write(net::tcp::construct_buffer(buf, buf + len));
  }

  inline void TLS_stream::tls_read(buffer_t buffer)
  {
    uint8_t* buf = buffer->data();
    int      len = buffer->size();

    while (len > 0)
    {
      int n = BIO_write(this->m_bio_rd, buf, len);
      if (UNLIKELY(n < 0)) {
        this->close();
        return;
      }
      buf += n;
      len -= n;

      // if we aren't finished initializing session
      if (UNLIKELY(!handshake_completed()))
      {
        int num = SSL_accept(this->m_ssl);
        auto status = this->status(num);

        // OpenSSL wants to write
        if (status == STATUS_WANT_IO)
        {
          tls_perform_stream_write();
        }
        else if (status == STATUS_FAIL)
        {
          this->close();
          return;
        }
        // nothing more to do if still not finished
        if (handshake_completed() == false) return;
        // handshake success
        if (m_on_connect) m_on_connect(*this);
      }

      // read decrypted data
      do {
        auto buf = net::tcp::construct_buffer(8192);
        n = SSL_read(this->m_ssl, buf->data(), buf->size());
        if (n > 0) {
          buf->resize(n);
          if (m_on_read) m_on_read(std::move(buf));
        }
      } while (n > 0);
      // this goes here?
      if (UNLIKELY(this->is_closing() || this->is_closed())) {
          this->close_callback_once();
          return;
      }

      auto status = this->status(n);
      // did peer request stream renegotiation?
      if (status == STATUS_WANT_IO)
      {
        do {
          n = tls_perform_stream_write();
        } while (n > 0);
      }
      else if (status == STATUS_FAIL)
      {
        this->close();
        return;
      }
    } // while it < end
  } // tls_read()

  inline int TLS_stream::tls_perform_stream_write()
  {
    char buffer[8192];
    int n = BIO_read(this->m_bio_wr, buffer, sizeof(buffer));
    if (LIKELY(n > 0))
    {
      m_transport->write(buffer, n);
      if (m_on_write) m_on_write(n);
      return n;
    }
    else if (UNLIKELY(!BIO_should_retry(this->m_bio_wr)))
    {
      this->close();
      return -1;
    }
    return 0;
  }

  inline void TLS_stream::close()
  {
    m_transport->close();
  }
  inline void TLS_stream::abort()
  {
    m_transport->abort();
    this->reset_callbacks();
  }
  inline void TLS_stream::close_callback_once()
  {
    auto func = std::move(m_on_close);
    m_on_close = nullptr; // FIXME: this is a bug in delegate
    if (func) func();
    // free captured resources
    this->reset_callbacks();
  }
  inline void TLS_stream::reset_callbacks()
  {
    this->m_on_close = nullptr;
    this->m_on_connect = nullptr;
    this->m_on_read = nullptr;
  }

  inline bool TLS_stream::handshake_completed() const noexcept
  {
    return SSL_is_init_finished(this->m_ssl);
  }
  inline TLS_stream::status_t TLS_stream::status(int n) const noexcept
  {
    int error = SSL_get_error(this->m_ssl, n);
    switch (error)
    {
    case SSL_ERROR_NONE:
        return STATUS_OK;
    case SSL_ERROR_WANT_WRITE:
    case SSL_ERROR_WANT_READ:
        return STATUS_WANT_IO;
    default:
        return STATUS_FAIL;
    }
  }
} // openssl
