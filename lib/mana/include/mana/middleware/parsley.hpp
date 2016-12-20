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

#ifndef MANA_MIDDLEWARE_PARSLEY_HPP
#define MANA_MIDDLEWARE_PARSLEY_HPP

#include <mana/attributes/json.hpp>
#include <mana/middleware.hpp>

namespace mana {
namespace middleware {

/**
 * @brief A vegan way to parse JSON Content in a response
 * @details TBC..
 *
 */
class Parsley : public Middleware {
public:

  Callback handler() override {
    return {this, &Parsley::process};
  }
  /**
   *
   */
  void process(mana::Request_ptr req, mana::Response_ptr, mana::Next next);

private:
  /**
   *
   */
  bool has_json(const mana::Request& req) const;
}; //< class Parsley

}} //< namespace mana::middleware

#endif //< JSON_PARSLEY_HPP
