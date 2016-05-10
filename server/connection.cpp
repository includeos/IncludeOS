#include "connection.hpp"
#include "server.hpp"

using namespace server;

Connection::Connection(Server& serv, Connection_ptr conn)
  : server_(serv), conn_(conn)
{
  conn_->read(BUFSIZE, OnData::from<Connection, &Connection::on_data>(this));
  conn_->onDisconnect(OnDisconnect::from<Connection, &Connection::on_disconnect>(this));
}

void Connection::on_data(buffer_t buf, size_t n) {
  if(!request_)
    request_ = std::make_shared<Request>(buf, n);

  response_ = std::make_shared<Response>(conn_);

  server_.process(request_, response_);

}

void Connection::on_disconnect(Connection_ptr, Disconnect) {
  close();
}

void Connection::close() {
  conn_->close();
  server_.close(this);
}
