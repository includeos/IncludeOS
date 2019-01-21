#include <net/tcp/connection.hpp>
#include <net/stream.hpp>

// TCP stream

namespace net::tcp
{
  /**
   * @brief      Exposes a TCP Connection as a Stream with only the most necessary features.
   *             May be overrided by extensions like TLS etc for additional functionality.
   */
  class Stream final : public net::Stream {
  public:
    static const uint16_t SUBID = 1;
    /**
     * @brief      Construct a Stream for a Connection ptr
     *
     * @param[in]  conn  The connection
     */
    Stream(Connection_ptr conn)
      : m_tcp{std::move(conn)}
    {
      // stream for a nullptr makes no sense
      Expects(m_tcp != nullptr);
      m_tcp->on_close({this, &Stream::close});
    }
    ~Stream()
    {
      this->reset_callbacks();
      m_tcp->close();
    }

    /**
     * @brief      Event when the stream is connected/established/ready to use.
     *
     * @param[in]  cb    The connect callback
     */
    void on_connect(ConnectCallback cb) override
    {
      m_tcp->on_connect(Connection::ConnectCallback::make_packed(
          [this, cb] (Connection_ptr conn)
          {
            // this will ensure at least close is called if the connect fails
            if (conn != nullptr) {
              cb(*this);
            }
            else {
              if (this->m_on_close) this->m_on_close();
            }
          }));
    }

    /**
     * @brief      Event when data is received.
     *
     * @param[in]  n     The size of the receive buffer
     * @param[in]  cb    The read callback
     */
    void on_read(size_t n, ReadCallback cb) override
    { m_tcp->on_read(n, cb); }

    /**
     * @brief      Event when data is received.
     *             Does not push data, just signals its presence.
     *
     * @param[in]  cb    The callback
     */
    void on_data(DataCallback cb) override {
      m_tcp->on_data(cb);
    };

    /**
     * @return The size of the next available chunk of data if any.
     */
    size_t next_size() override {
      return m_tcp->next_size();
    };

    /**
     * @return The next available chunk of data if any.
     */
    buffer_t read_next() override {
      return m_tcp->read_next();
    };


    /**
     * @brief      Event for when the Stream is being closed.
     *
     * @param[in]  cb    The close callback
     */
    void on_close(CloseCallback cb) override
    {
      m_on_close = std::move(cb);
    }

    /**
     * @brief      Event for when data has been written.
     *
     * @param[in]  cb    The write callback
     */
    void on_write(WriteCallback cb) override
    { m_tcp->on_write(cb); }

    /**
     * @brief      Async write of a data with a length.
     *
     * @param[in]  buf   data
     * @param[in]  n     length
     */
    void write(const void* buf, size_t n) override
    { m_tcp->write(buf, n); }

    /**
     * @brief      Async write of a shared buffer with a length.
     *
     * @param[in]  buffer  shared buffer
     * @param[in]  n       length
     */
    void write(buffer_t buffer) override
    { m_tcp->write(buffer); }

    /**
     * @brief      Async write of a string.
     *             Calls write(const void* buf, size_t n)
     *
     * @param[in]  str   The string
     */
    void write(const std::string& str) override
    { write(str.data(), str.size()); }

    /**
     * @brief      Closes the stream.
     */
    void close() override
    {
      auto onclose = std::move(this->m_on_close);
      m_tcp->reset_callbacks();
      m_tcp->close();
      if (onclose) onclose();
    }

    /**
     * @brief      Resets all callbacks.
     */
    void reset_callbacks() override
    { m_tcp->reset_callbacks(); }

    /**
     * @brief      Returns the streams local socket.
     *
     * @return     A TCP Socket
     */
    Socket local() const override
    { return m_tcp->local(); }

    /**
     * @brief      Returns the streams remote socket.
     *
     * @return     A TCP Socket
     */
    Socket remote() const override
    { return m_tcp->remote(); }

    /**
     * @brief      Returns a string representation of the stream.
     *
     * @return     String representation of the stream.
     */
    std::string to_string() const override
    { return m_tcp->to_string(); }

    /**
     * @brief      Determines if connected (established).
     *
     * @return     True if connected, False otherwise.
     */
    bool is_connected() const noexcept override
    { return m_tcp->is_connected(); }

    /**
     * @brief      Determines if writable. (write is allowed)
     *
     * @return     True if writable, False otherwise.
     */
    bool is_writable() const noexcept override
    { return m_tcp->is_writable(); }

    /**
     * @brief      Determines if readable. (data can be received)
     *
     * @return     True if readable, False otherwise.
     */
    bool is_readable() const noexcept override
    { return m_tcp->is_readable(); }

    /**
     * @brief      Determines if closing.
     *
     * @return     True if closing, False otherwise.
     */
    bool is_closing() const noexcept override
    { return m_tcp->is_closing(); }

    /**
     * @brief      Determines if closed.
     *
     * @return     True if closed, False otherwise.
     */
    bool is_closed() const noexcept override
    { return m_tcp->is_closed(); };

    int get_cpuid() const noexcept override;

    size_t serialize_to(void* p, const size_t) const override {
      return m_tcp->serialize_to(p);
    }
    uint16_t serialization_subid() const override {
      return SUBID;
    }

    Stream* transport() noexcept override {
      return nullptr;
    }

    Connection_ptr tcp() {
      return this->m_tcp;
    }

  protected:
    Connection_ptr m_tcp;
    CloseCallback  m_on_close = nullptr;

  }; // < class Connection::Stream

}
