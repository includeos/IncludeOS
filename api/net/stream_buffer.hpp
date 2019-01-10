#ifndef STREAMBUFFERR_HPP
#define STREAMBUFFERR_HPP
#include <net/stream.hpp>
#include <queue>
namespace net {
  class StreamBuffer : public net::Stream
  {
  public:
    using buffer_t = os::mem::buf_ptr;
    using Ready_queue  = std::deque<buffer_t>;
    //virtual ~StreamBuffer();

    void on_connect(ConnectCallback cb) override {
      m_on_connect = std::move(cb);
    }

    void on_read(size_t, ReadCallback cb) override {
      m_on_read = std::move(cb);
    }
    void on_data(DataCallback cb) override {
      m_on_data = std::move(cb);
    }
    size_t next_size() override;

    buffer_t read_next() override;

    void on_close(CloseCallback cb) override {
      m_on_close = std::move(cb);
    }
    void on_write(WriteCallback cb) override {
      m_on_write = std::move(cb);
    }

    void signal_data();

    bool read_congested() const noexcept
    { return m_read_congested; }

    bool write_congested() const noexcept
    { return m_write_congested; }

    /**
     * @brief Construct a shared read vector used by streams
     * If allocation failed congestion flag is set
     *
     * @param construction parameters
     *
     * @return nullptr on failure, shared_ptr to buffer on success
     */
    template <typename... Args>
    buffer_t construct_read_buffer(Args&&... args)
    {
      return construct_buffer_with_flag(m_read_congested,std::forward<Args> (args)...);
    }

    /**
     * @brief Construct a shared write vector used by streams
     * If allocation failed congestion flag is set
     *
     * @param construction parameters
     *
     * @return nullptr on failure, shared_ptr to buffer on success
     */
    template <typename... Args>
    buffer_t construct_write_buffer(Args&&... args)
    {
      return construct_buffer_with_flag(m_write_congested,std::forward<Args> (args)...);
    }

  protected:
    void closed()
    { if (m_on_close) m_on_close(); }
    void connected()
    { if (m_on_connect) m_on_connect(*this); }
    void stream_on_write(int n)
    { if (m_on_write) m_on_write(n); }
    void enqueue_data(buffer_t data)
    { m_send_buffers.push_back(data); }

    CloseCallback getCloseCallback() { return std::move(this->m_on_close); }

    void reset_callbacks() override
    {
      //remove queue and reset congestion flags and busy flag ??
      this->m_on_close = nullptr;
      this->m_on_connect = nullptr;
      this->m_on_read  = nullptr;
      this->m_on_write = nullptr;
      this->m_on_data = nullptr;
    }
  private:

    bool  m_write_congested= false;
    bool  m_read_congested = false;

    ConnectCallback  m_on_connect = nullptr;
    ReadCallback     m_on_read    = nullptr;
    DataCallback     m_on_data    = nullptr;
    WriteCallback    m_on_write   = nullptr;
    CloseCallback    m_on_close   = nullptr;
    Ready_queue      m_send_buffers;

    /**
     * @brief Construct a shared vector and set congestion flag if allocation fails
     *
     * @param flag the flag to set true or false on allocation failure
     * @param args arguments to constructing the buffer
     * @return nullptr on failure , shared pointer to buffer on success
     */

    template <typename... Args>
    buffer_t construct_buffer_with_flag(bool &flag,Args&&... args)
    {
      buffer_t buffer;
      try
      {
        buffer = std::make_shared<os::mem::buffer> (std::forward<Args> (args)...);
        flag = false;
      }
      catch (std::exception &e)
      {
        flag = true;
        return nullptr;
      }
      return buffer;
    }


  }; // < class StreamBuffer

  inline size_t StreamBuffer::next_size()
  {
    if (not m_send_buffers.empty()) {
      return m_send_buffers.front()->size();
    }
    return 0;
  }

  inline StreamBuffer::buffer_t StreamBuffer::read_next()
  {

    if (not m_send_buffers.empty()) {
      auto buf = m_send_buffers.front();
      m_send_buffers.pop_front();
      return buf;
    }
    return nullptr;
  }

  inline void StreamBuffer::signal_data()
  {
    if (not m_send_buffers.empty())
    {
      if (m_on_data != nullptr){
        //on_data_callback();
        m_on_data();
        if (not m_send_buffers.empty()) {
          // FIXME: Make sure this event gets re-triggered
          // For now the user will have to make sure to re-read later if they couldn't
        }
      }
      else if (m_on_read != nullptr)
      {
        for (auto buf : m_send_buffers) {
          // Pop each time, in case callback leads to another call here.
          m_send_buffers.pop_front();
          m_on_read(buf);
        }
      }
    }
  }
} // namespace net
#endif // STREAMBUFFERR_HPP
