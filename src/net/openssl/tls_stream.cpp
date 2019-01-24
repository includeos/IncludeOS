#include <net/openssl/tls_stream.hpp>

using namespace openssl;

TLS_stream::TLS_stream(SSL_CTX* ctx, Stream_ptr t, bool outgoing)
  : m_transport(std::move(t))
{
  ERR_clear_error(); // prevent old errors from mucking things up
  this->m_bio_rd = BIO_new(BIO_s_mem());
  this->m_bio_wr = BIO_new(BIO_s_mem());
  assert(ERR_get_error() == 0 && "Initializing BIOs");
  this->m_ssl = SSL_new(ctx);
  assert(this->m_ssl != nullptr);
  assert(ERR_get_error() == 0 && "Initializing SSL");
  // TLS server-mode
  if (outgoing == false)
      SSL_set_accept_state(this->m_ssl);
  else
      SSL_set_connect_state(this->m_ssl);

  SSL_set_bio(this->m_ssl, this->m_bio_rd, this->m_bio_wr);

  // always-on callbacks
  m_transport->on_data({this,&TLS_stream::handle_data});
  m_transport->on_close({this, &TLS_stream::close_callback_once});

  // start TLS handshake process
  if (outgoing == true)
  {
    if (this->tls_perform_handshake() < 0) return;
  }
}
TLS_stream::TLS_stream(Stream_ptr t, SSL* ssl, BIO* rd, BIO* wr)
  : m_transport(std::move(t)), m_ssl(ssl), m_bio_rd(rd), m_bio_wr(wr)
{
  // always-on callbacks
  m_transport->on_data({this, &TLS_stream::handle_data});
  m_transport->on_close({this, &TLS_stream::close_callback_once});
}
TLS_stream::~TLS_stream()
{
  assert(m_busy == false && "Cannot delete stream while in its call stack");
  SSL_free(this->m_ssl);
}

void TLS_stream::write(buffer_t buffer)
{

  if (UNLIKELY(this->is_connected() == false)) {
    TLS_PRINT("::write() called on closed stream\n");
    return;
  }
  int n = SSL_write(this->m_ssl, buffer->data(), buffer->size());
  auto status = this->status(n);
  if (status == STATUS_FAIL) {
    TLS_PRINT("::write() Fail status %d\n",n);
    this->close();
    return;
  }

  do {
    n = tls_perform_stream_write();
  } while (n > 0);
}

void TLS_stream::write(const std::string& str)
{
  //TODO handle failed alloc
  write(net::StreamBuffer::construct_write_buffer(str.data(),str.data()+str.size()));
}

void TLS_stream::write(const void* data, const size_t len)
{
  //TODO handle failed alloc
  auto* buf = static_cast<const uint8_t*> (data);
  write(net::StreamBuffer::construct_write_buffer(buf, buf + len));
}

int TLS_stream::decrypt(const void *indata, int size)
{
  int n = BIO_write(this->m_bio_rd, indata, size);
  if (UNLIKELY(n < 0)) {
    //TODO can we handle this more gracefully?
    TLS_PRINT("BIO_write failed\n");
    this->close();
    return -1;
  }

  // if we aren't finished initializing session
  if (UNLIKELY(!handshake_completed()))
  {
    int num = SSL_do_handshake(this->m_ssl);
    auto status = this->status(num);

    // OpenSSL wants to write
    if (status == STATUS_WANT_IO)
    {
      tls_perform_stream_write();
    }
    else if (status == STATUS_FAIL)
    {
      if (num < 0) {
        TLS_PRINT("TLS_stream::SSL_do_handshake() returned %d\n", num);
        #ifdef VERBOSE_OPENSSL
          ERR_print_errors_fp(stdout);
        #endif
      }
      this->close();
      return -1;
    }
    // nothing more to do if still not finished
    if (handshake_completed() == false) return 0;
    // handshake success
    this->m_busy=true;
    connected();
    this->m_busy=false;

    if (this->m_deferred_close) {
      TLS_PRINT("::read() close on m_deferred_close after tls_perform_stream_write\n");
      this->close();
      return -1;
    }
  }
  return n;
}

int TLS_stream::send_decrypted()
{
  int n;
  // read decrypted data
  do {
    //TODO "increase the size or constructor based ??")
    auto buffer=StreamBuffer::construct_read_buffer(8192);
    if (!buffer) return 0;
    n = SSL_read(this->m_ssl,buffer->data(),buffer->size());
    if (n > 0) {
      buffer->resize(n);
      enqueue_data(buffer);
    }
  } while (n > 0);
  return n;
}

void TLS_stream::handle_read_congestion()
{
  //Ordering could be different
  send_decrypted(); //decrypt any incomplete
  this->m_busy=true;
  signal_data(); //send any pending
  this->m_busy=false;

  if (this->m_deferred_close) {
    TLS_PRINT("::read() close on m_deferred_close after tls_perform_stream_write\n");
    this->close();
    return;
  }
}

void TLS_stream::handle_write_congestion()
{
  //this should resolve the potential malloc congestion
  //might be missing some TLS signalling but without malloc we cant do that either
  while(tls_perform_stream_write() >  0);
}
void TLS_stream::handle_data()
{
  while (m_transport->next_size() > 0)
  {
    if (UNLIKELY(read_congested())){
      break;
    }
    auto buffer = m_transport->read_next();
    if (UNLIKELY(!buffer)) break;
    bool closed = tls_read(buffer);
    // tls_read can close this stream
    if (closed) break;
  }
}

bool TLS_stream::tls_read(buffer_t buffer)
{
  if (buffer == nullptr ) {
    return false;
  }
  ERR_clear_error();
  uint8_t* buf_ptr = buffer->data();
  int      len = buffer->size();

  while (len > 0)
  {
    if (this->m_deferred_close) {
      TLS_PRINT("::read() close on m_deferred_close");
      this->close();
      return true;
    }

    const int decrypted_bytes = decrypt(buf_ptr, len);
    if (UNLIKELY(decrypted_bytes == 0)) {
      return false;
    }
    else if (UNLIKELY(decrypted_bytes < 0)) {
      return true;
    }
    buf_ptr += decrypted_bytes;
    len -= decrypted_bytes;

    //enqueues decrypted data
    int ret=send_decrypted();

    // this goes here?
    if (UNLIKELY(this->is_closing() || this->is_closed())) {
      TLS_PRINT("TLS_stream::SSL_read closed during read\n");
      return true;
    }
    if (this->m_deferred_close) {
      TLS_PRINT("::read() close on m_deferred_close");
      this->close();
      return true;
    }

    auto status = this->status(ret);
    // did peer request stream renegotiation?
    if (status == STATUS_WANT_IO)
    {
      TLS_PRINT("::read() STATUS_WANT_IO\n");
      int ret;
      do {
        ret = tls_perform_stream_write();
      } while (ret > 0);
    }
    else if (status == STATUS_FAIL)
    {
      TLS_PRINT("::read() close on STATUS_FAIL after tls_perform_stream_write\n");
      this->close();
      return true;
    }

  } // while it < end

  //forward data
  this->m_busy=true;
  signal_data();
  this->m_busy=false;

  // check deferred closing
  if (this->m_deferred_close) {
    TLS_PRINT("::read() close on m_deferred_close after tls_perform_stream_write\n");
    this->close();
    return true;
  }
  return false;
} // tls_read()

int TLS_stream::tls_perform_stream_write()
{
  ERR_clear_error();
  int pending = BIO_ctrl_pending(this->m_bio_wr);
  if (pending > 0)
  {
    TLS_PRINT("::tls_perform_stream_write() pending=%d bytes\n",pending);
    auto buffer = net::StreamBuffer::construct_write_buffer(pending);
    if (buffer == nullptr) {
      return 0;
    }
    int n = BIO_read(this->m_bio_wr, buffer->data(), buffer->size());
    assert(n == pending);
    //What if we cant write..
    if (m_transport->is_writable())
    {
      m_transport->write(buffer);

      this->m_busy = true;
      stream_on_write(n);
      this->m_busy = false;

      if (this->m_deferred_close) {
        TLS_PRINT("::read() close on m_deferred_close after tls_perform_stream_write\n");
        this->close(); return 0;
      }
    }

    if (UNLIKELY((pending = BIO_ctrl_pending(this->m_bio_wr)) > 0))
    {
      return pending;
    }
    return 0;
  }

  BIO_read(this->m_bio_wr, nullptr, 0);

  if (!BIO_should_retry(this->m_bio_wr))
  {
    TLS_PRINT("::tls_perform_stream_write() close on !BIO_should_retry\n");
    this->close();
    return -1;
  }
  return 0;
}

int TLS_stream::tls_perform_handshake()
{
  ERR_clear_error(); // prevent old errors from mucking things up
  // will return -1:SSL_ERROR_WANT_WRITE
  int ret = SSL_do_handshake(this->m_ssl);
  int n = this->status(ret);
  ERR_print_errors_fp(stderr);
  if (n == STATUS_WANT_IO)
  {
    do {
      n = tls_perform_stream_write();
      if (n < 0) {
        TLS_PRINT("TLS_stream::tls_perform_handshake() stream write failed\n");
      }
    } while (n > 0);
    return n;
  }
  else {
    TLS_PRINT("TLS_stream::tls_perform_handshake() returned %d\n", ret);
    this->close();
    return -1;
  }
}

void TLS_stream::close()
{
  TLS_PRINT("TLS_stream::close()\n");
  //ERR_clear_error();
  if (this->m_busy) {
    TLS_PRINT("TLS_stream::close() deferred\n");
    this->m_deferred_close = true; return;
  }
  CloseCallback func = getCloseCallback();
  this->reset_callbacks();
  if (m_transport->is_connected())
  {
      m_transport->close();
      m_transport->reset_callbacks(); // ???
  }
  if (func) func();
}
void TLS_stream::close_callback_once()
{
  TLS_PRINT("TLS_stream::close_callback_once() \n");
  if (this->m_busy) {
    TLS_PRINT("TLS_stream::close_callback_once() deferred\n");
    this->m_deferred_close = true; return;
  }
  CloseCallback func = getCloseCallback();
  this->reset_callbacks();
  if (func) func();
}

bool TLS_stream::handshake_completed() const noexcept
{
  return SSL_is_init_finished(this->m_ssl);
}
TLS_stream::status_t TLS_stream::status(int n) const noexcept
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
