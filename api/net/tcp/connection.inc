//-*- C++ -*-

namespace net {
namespace tcp {

inline Connection& Connection::on_connect(ConnectCallback cb) {
  on_connect_ = cb;
  return *this;
}

inline Connection& Connection::on_read(size_t recv_bufsz, ReadCallback cb)
{
  _on_read(recv_bufsz, cb);
  return *this;
}

inline Connection& Connection::on_data(DataCallback cb) {
  _on_data(cb);
  return *this;
}

inline Connection& Connection::on_disconnect(DisconnectCallback cb) {
  on_disconnect_ = cb;
  return *this;
}

inline Connection& Connection::on_write(WriteCallback cb) {
  writeq.on_write(std::move(cb));
  return *this;
}

inline Connection& Connection::on_close(CloseCallback cb) {
  on_close_ = cb;
  return *this;
}

inline Connection& Connection::set_on_read_callback(ReadCallback cb) {
  Expects(read_request != nullptr && "on_read hasn't been called before.");
  read_request->on_read_callback = cb;
  return *this;
}

inline Connection& Connection::_on_cleanup(CleanupCallback cb) {
  _on_cleanup_ = cb;
  return *this;
}

inline buffer_t Connection::read_next() {
  if (UNLIKELY(read_request == nullptr)) {
    return nullptr;
  }
  return read_request->read_next();
}

inline size_t Connection::next_size() {
  if (UNLIKELY(read_request == nullptr)) {
    return 0;
  }
  return read_request->next_size();
}

inline void Connection::write(const void* buf, size_t n) {
  this->write(tcp::construct_buffer((uint8_t*) buf, (uint8_t*) buf + n));
}

inline void Connection::write(const std::string& str) {
  this->write(str.data(), str.size());
}

inline void Connection::abort() {
  state_->abort(*this);
  signal_close();
}

}; // < namespace tcp
}; // < namespace net
