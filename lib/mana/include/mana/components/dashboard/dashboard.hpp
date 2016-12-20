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
#ifndef DASHBOARD_DASHBOARD_HPP
#define DASHBOARD_DASHBOARD_HPP

#include <mana/router.hpp>
#include <mana/attributes/json.hpp>
#include "component.hpp"
#include "common.hpp"

namespace mana {
namespace dashboard {

class Dashboard {

  using Component_ptr = std::shared_ptr<Component>;
  using ComponentCollection = std::unordered_map<std::string, Component_ptr>;

public:
  Dashboard(size_t buffer_capacity = 4096);

  const mana::Router& router() const
  { return router_; }

  void add(Component_ptr);

  template <typename Comp, typename... Args>
  inline void construct(Args&&...);

private:

  mana::Router router_;
  WriteBuffer buffer_;
  Writer writer_;

  ComponentCollection components_;

  void setup_routes();

  void serve(mana::Request_ptr, mana::Response_ptr);
  void serialize(Writer&);

  void send_buffer(mana::Response&);
  void reset_writer();

};

inline void Dashboard::add(Component_ptr c) {
  components_.emplace(c->key(), c);

  // A really simple way to setup routes, only supports read (GET)
  router_.on_get("/" + c->key(),
  [this, c] (mana::Request_ptr, mana::Response_ptr res)
  {
    c->serialize(writer_);
    send_buffer(*res);
  });
}

template <typename Comp, typename... Args>
inline void Dashboard::construct(Args&&... args) {
  static_assert(std::is_base_of<Component, Comp>::value, "Template type is not a Component");

  add(std::make_unique<Comp>(std::forward<Args>(args)...));
}

} // < namespace dashboard
} // < namespace mana

#endif
