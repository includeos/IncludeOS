#include "server.hpp"

// #define DEBUG

using namespace server;

Server::Server() {
  initialize();
}

Server::Server(IP_Stack stack) : inet_(stack) {}

net::Inet4<VirtioNet>& Server::ip_stack() const {
  return *inet_;
}

Router& Server::router() noexcept {
  return router_;
}

void Server::listen(Port port) {
  printf("Listening to port %i \n", port);

  Server& server = *this;

  inet_->tcp().bind(port).onConnect([&](auto conn) {

    conn->read(1500, [conn, &server](net::TCP::buffer_t buf, size_t n) {
        auto data = std::string((char*)buf.get(), n);
        debug("Received data: %s\n", data.c_str());

      // Create request / response objects for callback
      Request  req {data};
      std::shared_ptr<Response> res = std::make_shared<Response>(conn);

      // Get and call the callback
      server.router_[{req.method(), req.uri()}](req, res);
      //-------------------------------
    }); // < read

    // if user sends FIN, we do also want to close.
    conn->onDisconnect([](auto conn, auto) {
      printf("Closing %s\n", conn->to_string().c_str());
      conn->close();
    });
  }); // < onConnect
}

void Server::initialize() {
  auto& eth0 = hw::Dev::eth<0,VirtioNet>();
  //-------------------------------
  inet_ = std::make_shared<net::Inet4<VirtioNet>>(eth0);
  //-------------------------------
  inet_->network_config({ 10,0,0,42 },     // IP
      { 255,255,255,0 }, // Netmask
      { 10,0,0,1 },      // Gateway
      { 8,8,8,8 });      // DNS
}

void Server::close(Connection*) {

}

void Server::process(Request_ptr, Response_ptr) {

}
