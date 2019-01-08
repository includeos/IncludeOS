
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
  m_transport->on_read(8192, {this, &TLS_stream::tls_read});
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
  m_transport->on_read(8192, {this, &TLS_stream::tls_read});
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
    TLS_PRINT("TLS_stream::write() called on closed stream\n");
    return;
  }

  int n = SSL_write(this->m_ssl, buffer->data(), buffer->size());
  auto status = this->status(n);
  if (status == STATUS_FAIL) {
    this->close();
    return;
  }

  auto alloc=buffer->get_allocator();


  //if stored ptr is nullptr then create it
  if (UNLIKELY(!tls_buffer))
  {
    //perform initial pre alloc quite large
    try {
      printf("Creating initial pmr buffer size %zu\n",buffer->size()*2);
      tls_buffer=std::make_shared<std::pmr::vector<uint8_t>>(buffer->size()*2,alloc);
    }
    catch (std::exception &e) //this is allways a failed to allocate!!
    {
      //could attempt buffer reuse..
      printf("Failed to allocate to pre buffer\n");
      return;
    }
  }

  //release memory
  buffer->clear();
  //reset ?
  //delete buffer;
  //first Buffer R belongs to US

  //if shared ptr is unset create initial buffer
  //Not sane..
  do {
    n = tls_write_to_stream(alloc);
  } while (n > 0);
}
void TLS_stream::write(const std::string& str)
{
  write(net::Stream::construct_buffer(str.data(), str.data() + str.size()));
}
void TLS_stream::write(const void* data, const size_t len)
{
  auto* buf = static_cast<const uint8_t*> (data);
  write(net::Stream::construct_buffer(buf, buf + len));
}

void TLS_stream::tls_read(buffer_t buffer)
{
  ERR_clear_error();
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
        return;
      }
      // nothing more to do if still not finished
      if (handshake_completed() == false) return;
      // handshake success
      if (m_on_connect) m_on_connect(*this);
    }

    // read decrypted data
    do {
      char temp[8192];
      n = SSL_read(this->m_ssl, temp, sizeof(temp));
      if (n > 0) {
        auto buf = net::Stream::construct_buffer(temp, temp + n);
        if (m_on_read) {
          this->m_busy = true;
          m_on_read(std::move(buf));
          this->m_busy = false;
        }
      }
    } while (n > 0);
    // this goes here?
    if (UNLIKELY(this->is_closing() || this->is_closed())) {
      TLS_PRINT("TLS_stream::SSL_read closed during read\n");
      return;
    }
    if (this->m_deferred_close) {
      this->close(); return;
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
    // check deferred closing
    if (this->m_deferred_close) {
      this->close(); return;
    }

  } // while it < end
} // tls_read()


//TODO pass allocator !!
int TLS_stream::tls_write_to_stream(Alloc &alloc/*buffer_t buffer*/)
{
  ERR_clear_error();
  int pending = BIO_ctrl_pending(this->m_bio_wr);
  printf("pending: %d\n", pending);
  if (pending > 0)
  {
    //TODO create a preallocated buffer ?
    //this allocates in the buffer..
    //tls_write_to_stream

//    auto buffer = net::Stream::construct_buffer(pending);
    //printf("buffer size %zu\n",buffer->size());
    if (pending != tls_buffer->size())
    {
      //try catch only when
      /*if (UNLIKELY(pending > tls_buffer->capacity()))
      {
        try
      }*/
      //printf("Increasing size of tls_buffer to %zu\n",pending);
      try
      {
        tls_buffer->resize(pending);
      }
      catch (std::exception &e)
      {
        //release whats allocated
        tls_buffer->clear();
        //set nullptr
        tls_buffer=nullptr;
        return 0;
      }
    }
    //printf("buffer size %zu\n",tls_buffer->size());
    int n = BIO_read(this->m_bio_wr, tls_buffer->data(), tls_buffer->size());
    assert(n == pending);
    //printf("transport write\n");
    m_transport->write(tls_buffer);

    try
    {
      printf("Assigning new buffer to tls_buffer\n");
      tls_buffer = std::make_shared<std::pmr::vector<uint8_t>>(pending,alloc);
    }
    catch (std::exception &e)
    {
      printf("Failed to allocate tls_buffer setting shared_ptr to nullptr\n");
      //move problem up the chain by setting the shared ptr to a nullptr
      tls_buffer = nullptr;//std::make_shared<std::pmr::vector<uint8_t>>(0);
      //return 0
    }

    if (m_on_write) {
      this->m_busy = true;
      m_on_write(n);
      this->m_busy = false;
    }
    return n;
  }
  else {
    BIO_read(this->m_bio_wr, nullptr, 0);
  }
  if (!BIO_should_retry(this->m_bio_wr))
  {
    this->close();
    return -1;
  }
  return 0;
}

//When no pmr buffer is passed use malloc
int TLS_stream::tls_perform_stream_write()
{
  ERR_clear_error();
  int pending = BIO_ctrl_pending(this->m_bio_wr);
  printf("pending: %d\n", pending);
  if (pending > 0)
  {

    auto buffer = std::make_shared<std::pmr::vector<uint8_t>>(pending);//(std::vector<uint8_t>(pending));
    if (buffer->size() < pending)
    {
      printf("Buffer %zu < pending %zu\n",buffer->size(),pending);
      //descope buffer
      return 0;
    }
    //auto buffer = net::Stream::construct_buffer(pending);
    printf("buffer size %zu\n",buffer->size());
    int n = BIO_read(this->m_bio_wr, buffer->data(), buffer->size());
    assert(n == pending);
    printf("transport write\n");
    m_transport->write(buffer);
    if (m_on_write) {
      this->m_busy = true;
      m_on_write(n);
      this->m_busy = false;
    }
    return n;
  }
  else {
    BIO_read(this->m_bio_wr, nullptr, 0);
  }
  if (!BIO_should_retry(this->m_bio_wr))
  {
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
  //ERR_clear_error();
  if (this->m_busy) {
    this->m_deferred_close = true; return;
  }
  CloseCallback func = std::move(this->m_on_close);
  this->reset_callbacks();
  if (m_transport->is_connected())
      m_transport->close();
  if (func) func();
}
void TLS_stream::close_callback_once()
{
  if (this->m_busy) {
    this->m_deferred_close = true; return;
  }
  CloseCallback func = std::move(this->m_on_close);
  this->reset_callbacks();
  if (func) func();
}
void TLS_stream::reset_callbacks()
{
  this->m_on_close = nullptr;
  this->m_on_connect = nullptr;
  this->m_on_read  = nullptr;
  this->m_on_write = nullptr;
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
