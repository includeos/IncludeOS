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
  printf("<Connection> @on_data, size=%u\n", n);
  // if it's a new request
  if(!request_) {
    try {
      request_ = std::make_shared<Request>(buf, n);
      // return early to read payload
      if((request_->method() == http::POST or request_->method() == http::PUT)
        and request_->content_length() > request_->get_body().size())
        return;
    } catch(...) {
      printf("<Connection> Error - exception thrown when creating Request???\n");
      close();
      return;
    }
  }
  // else we assume it's payload
  else {
    request_->add_body(request_->get_body() + std::string((const char*)buf.get(), n));
    // if we haven't received all data promised
    if(request_->content_length() > request_->get_body().size())
      return;
  }


  printf("<Connection:[%s]> Incoming Request [@%s:%s]\n",
    conn_->remote().to_string().c_str(),
    http::method::str(request_->method()).c_str(),
    request_->uri().path().c_str());

  response_ = std::make_shared<Response>(conn_);



  //std::cout << "Raw data: " << buf << " <<< End raw data.\n";

  server_.process(request_, response_);
  request_ = nullptr;

}

void Connection::on_disconnect(Connection_ptr, Disconnect) {
  close();
}

void Connection::close() {
  conn_->close();
  server_.close(idx_);
}
