// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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

#include "transport.hpp"
#include "common.hpp"

namespace uplink {

Header Header::parse(const char* data)
{
  Expects(data != nullptr);
  return Header{
    static_cast<Transport_code>(data[0]), 
    *(reinterpret_cast<const uint32_t*>(&data[1]))
  };
}

Transport_parser::Transport_parser(Transport_complete cb)
  : on_complete{std::move(cb)}, on_header{nullptr}, transport_{nullptr}
{
  Expects(on_complete != nullptr);
}

void Transport_parser::parse(const char* data, size_t len)
{
  if(transport_ != nullptr)
  {
    transport_->load_cargo(data, len);
  }
  else
  {
    transport_ = std::make_unique<Transport>(Header::parse(data));
    
    if(on_header)
      on_header(transport_->header());

    len -= sizeof(Header);

    transport_->load_cargo(data + sizeof(Header), len);
  }

  if(transport_->is_complete())
    on_complete(std::move(transport_));
}

}

