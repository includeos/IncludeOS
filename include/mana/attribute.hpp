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

#ifndef SERVER_ATTRIBUTE_HPP
#define SERVER_ATTRIBUTE_HPP

#include <memory>

namespace server {

class Attribute;
using Attribute_ptr = std::shared_ptr<Attribute>;
using AttrType = size_t;

class Attribute {

public:
  template <typename A>
  static void register_attribute();

  template <typename A>
  static AttrType type();

private:
  static AttrType next_attr_type() {
    static AttrType counter;
    return ++counter;
  }
};

template <typename A>
void Attribute::register_attribute() {
  A::type(Attribute::next_attr_type());
}

template <typename A>
AttrType Attribute::type() {
  static_assert(std::is_base_of<Attribute, A>::value, "A is not an Attribute");
  static AttrType id = Attribute::next_attr_type();
  return id;
}


}; // < namespace server

#endif
