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

  inet_->tcp().bind(port).onConnect(OnConnect::from<Server, &Server::connect>(this));
}

void Server::connect(net::TCP::Connection_ptr conn) {
  printf("<Server> Connection from %s\n", conn->remote().to_string().c_str());
  // if there is a free spot in connections
  if(free_idx_.size() > 0) {
    auto idx = free_idx_.back();
    Ensures(connections_[idx] == nullptr);
    connections_[idx] = std::make_shared<Connection>(*this, conn, idx);
    free_idx_.pop_back();
  }
  // if not, add a new shared ptr
  else {
    connections_.emplace_back(new Connection{*this, conn, connections_.size()});
  }
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

void Server::close(size_t idx) {
  connections_[idx] = nullptr;
  free_idx_.push_back(idx);
}

void Server::process(Request_ptr req, Response_ptr res) {

  auto next = std::make_shared<next_t>();
  auto it_ptr = std::make_shared<MiddlewareStack::iterator>(middleware_.begin());
  // setup Next callback
  *next = [this, it_ptr, next, req, res] {
    // derefence the the pointer to the iterator
    auto& it = *it_ptr;
    // while there is more to do
    if(it != middleware_.end()) {
      // dereference the function
      auto& func = *it;
      // advance the iterator for the next next call
      it++;
      // execute the function
      func(req, res, next);
    }
    // no more middleware, proceed with route processing
    else {
      process_route(req, res);
    }
  };
  // get the party started..
  (*next)();
}

void Server::process_route(Request_ptr req, Response_ptr res) {
  try {
    router_.match(req->method(), req->uri().path())(req, res);
  }
  catch (Router_error err) {
    printf("<Server> Router_error: %s - Responding with 404.\n", err.what());
    res->set_status_code(http::Not_Found);
    res->send(true); // active close
  }
}

void Server::use(Callback middleware) {
  middleware_.push_back(middleware);
}
