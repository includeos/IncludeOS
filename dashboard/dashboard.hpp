#ifndef DASHBOARD_HPP
#define DASHBOARD_HPP

#include <os>
#include <delegate>
#include <server.hpp>
#include <json.hpp>

class Dashboard {
  using Buffer = rapidjson::StringBuffer;
  using Writer = rapidjson::Writer<Buffer>;
  using RouteCallback = delegate<void(server::Request_ptr, server::Response_ptr)>;

public:
  Dashboard();

  const server::Router& router() const
  { return router_; }

private:

  server::Router router_;
  Buffer buffer_;
  Writer writer_;
  const int stack_samples;

  void setup_routes();

  void serve_all(server::Request_ptr, server::Response_ptr);
  void serve_memmap(server::Request_ptr, server::Response_ptr);
  void serve_statman(server::Request_ptr, server::Response_ptr);
  void serve_stack_sampler(server::Request_ptr, server::Response_ptr);

  void serialize_memmap(Writer&) const;
  void serialize_statman(Writer&) const;
  void serialize_stack_sampler(Writer&) const;

  void send_buffer(server::Response_ptr);
  void reset_writer();

};



class Stats {

};


class Status {

};

#endif
