#include "connection.hpp"
#include "server.hpp"

using namespace server;

Connection::Connection(Server& serv, Connection_ptr conn, size_t idx)
  : server_(serv), conn_(conn), idx_(idx)
{
  conn_->read(BUFSIZE, OnData::from<Connection, &Connection::on_data>(this));
  conn_->onDisconnect(OnDisconnect::from<Connection, &Connection::on_disconnect>(this));
}

void Connection::on_data(buffer_t buf, size_t n) {
  request_ = std::make_shared<Request>(buf, n);

  printf("<Connection:[%s]> Incoming Request [ %s ]\n",
    conn_->remote().to_string().c_str(), request_->uri().path().c_str());

  response_ = std::make_shared<Response>(conn_);



  //std::cout << "Raw data: " << buf << " <<< End raw data.\n";

  server_.process(request_, response_);

}

void Connection::on_disconnect(Connection_ptr, Disconnect) {
  close();
}

void Connection::close() {
  conn_->close();
  server_.close(idx_);
}
