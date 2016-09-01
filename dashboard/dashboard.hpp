// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
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
  void serialize_status(Writer&) const;

  void send_buffer(server::Response_ptr);
  void reset_writer();

};

class Status {

};

#endif
