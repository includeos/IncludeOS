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

#ifndef SERVER_MIDDLEWARE_HPP
#define SERVER_MIDDLEWARE_HPP

#include "request.hpp"
#include "response.hpp"


namespace server {

class Middleware;
using Middleware_ptr = std::shared_ptr<Middleware>;

using next_t = delegate<void()>;
using Next = std::shared_ptr<next_t>;
using Callback = delegate<void(Request_ptr, Response_ptr, Next)>;

class Middleware {
public:

  virtual void process(Request_ptr, Response_ptr, Next) = 0;

  Callback callback() {
    return Callback::from<Middleware, &Middleware::process>(this);
  }

  virtual void onMount(const std::string& path) {
    mountpath_ = path;
  }

protected:
  std::string mountpath_;

};

}; // << namespace server


#endif
