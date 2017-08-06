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
#ifndef NET_TCP_READ_REQUEST_HPP
#define NET_TCP_READ_REQUEST_HPP

#include "read_buffer.hpp"
#include <delegate>

namespace net {
namespace tcp {

struct ReadRequest {
  using ReadCallback = delegate<void(buffer_t, size_t)>;

  ReadBuffer buffer;
  ReadCallback callback;

  ReadRequest()
    : buffer{nullptr, 0},
      callback{nullptr}
  {}

  ReadRequest(ReadBuffer buf)
    : ReadRequest(buf, nullptr)
  {}

  ReadRequest(ReadBuffer buf, ReadCallback cb)
    : buffer(buf),
      callback(cb)
  {}

  ReadRequest(size_t n)
    : buffer({new_shared_buffer(n), n}),
      callback({this, &ReadRequest::default_read_callback})
  {}

  void clean_up() {
    callback.reset();
  }

  void default_read_callback(buffer_t, size_t) {}
};

} // < namespace tcp
} // < namespace net

#endif // < NET_TCP_READ_REQUEST_HPP
