//-*- C++ -*-

namespace net {
namespace tcp {

inline Connection& Connection::on_connect(ConnectCallback cb) {
  on_connect_ = cb;
  return *this;
}

inline Connection& Connection::on_read(size_t recv_bufsz, ReadCallback cb)
{
  if(not read_request) {
    read_request = std::make_unique<ReadRequest>(recv_bufsz, seq_t(this->cb.RCV.NXT), cb);
  } else {
    read_request->buffer.reset(seq_t(this->cb.RCV.NXT), recv_bufsz);
    read_request->callback = cb;
  }
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

inline Connection& Connection::set_on_read_callback(ReadCallback cb) {
  Expects(read_request != nullptr && "on_read hasn't been called before.");
  read_request->callback = cb;
  return *this;
}

inline Connection& Connection::_on_cleanup(CleanupCallback cb) {
  _on_cleanup_ = cb;
  return *this;
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
