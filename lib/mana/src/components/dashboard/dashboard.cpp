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

#include <mana/components/dashboard/dashboard.hpp>

namespace mana {
namespace dashboard {

Dashboard::Dashboard(size_t buffer_capacity)
: router_(), buffer_(0, buffer_capacity), writer_(buffer_)
{
  setup_routes();
}

void Dashboard::setup_routes() {
  router_.on_get("/", {this, &Dashboard::serve});
}

void Dashboard::serve(Request_ptr, Response_ptr res) {
  writer_.StartObject();

  for(auto& pair : components_)
  {
    auto& key = pair.first;
    auto& comp = *(pair.second);

    writer_.Key(key.c_str());

    comp.serialize(writer_);
  }

  writer_.EndObject();
  send_buffer(*res);
}

void Dashboard::send_buffer(Response& res) {
  res.send_json(buffer_.GetString());
  reset_writer();
}

void Dashboard::reset_writer() {
  buffer_.Clear();
  writer_.Reset(buffer_);
}

}} //< namespace mana::dashboard
