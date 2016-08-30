//-*- C++ -*-

namespace net {
namespace tcp {

inline Connection& Connection::on_connect(ConnectCallback cb) {
  on_connect_ = cb;
  return *this;
}

inline Connection& Connection::on_read(size_t recv_bufsz, ReadCallback cb) {
  read(recv_bufsz, cb);
  return *this;
}

inline Connection& Connection::on_disconnect(DisconnectCallback cb) {
  on_disconnect_ = cb;
  return *this;
}

inline Connection& Connection::on_error(ErrorCallback callback) {
  on_error_ = callback;
  return *this;
}

inline Connection& Connection::on_packet_dropped(PacketDroppedCallback callback) {
  on_packet_dropped_ = callback;
  return *this;
}

inline Connection& Connection::on_rtx_timeout(RtxTimeoutCallback cb) {
  on_rtx_timeout_ = cb;
  return *this;
}

inline Connection& Connection::on_close(CloseCallback cb) {
  on_close_ = cb;
  return *this;
}

inline Connection& Connection::_on_cleanup(CleanupCallback cb) {
  _on_cleanup_ = cb;
  return *this;
}

inline void Connection::write(const void* buf, size_t n) {
  write(buf, n, WriteCallback::from<Connection,&Connection::default_on_write>(this), true);
}

inline void Connection::write(const void* buf, size_t n, WriteCallback callback) {
  write(buf, n, callback, true);
}

inline void Connection::write(buffer_t buffer, size_t n) {
  write({buffer, n, true}, WriteCallback::from<Connection,&Connection::default_on_write>(this));
}

inline void Connection::write(buffer_t buffer, size_t n, WriteCallback callback) {
  write({buffer, n, true}, callback);
}

inline void Connection::write(const std::string& str) {
  write(str.data(), str.size());
}

inline void Connection::write(const std::string& str, WriteCallback callback) {
  write(str.data(), str.size(), callback);
}

inline void Connection::abort() {
  state_->abort(*this);
  signal_close();
}

}; // < namespace tcp
}; // < namespace net
