//-*- C++ -*-

namespace net {
namespace tcp {

inline Connection& Connection::on_connect(ConnectCallback cb) {
  on_connect_ = cb;
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

inline Connection& Connection::on_packet_received(PacketReceivedCallback callback) {
  on_packet_received_ = callback;
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

}; // < namespace tcp
}; // < namespace net
