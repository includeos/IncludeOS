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

#ifndef MANA_PARAMS_HPP
#define MANA_PARAMS_HPP

#include <map>
#include <string>

namespace mana {

class ParamException : public std::exception {
  std::string msg;
  ParamException(){}

public:
  ParamException(const std::string& s) throw() : msg{s} {}
  const char* what() const throw() { return msg.c_str(); }
};  // < class ParamException

/**
 * @brief      Class for Request parameters.
 */
class Params {
public:
  bool insert(const std::string& name, const std::string& value) {
    auto ret = parameters.emplace(std::make_pair(name, value));
    return ret.second;
  }

  const std::string& get(const std::string& name) const {
    auto it = parameters.find(name);

    if (it != parameters.end()) // if found
      return it->second;

    throw ParamException{"Parameter with name " + name + " doesn't exist!"};
  }

private:
  std::map<std::string, std::string> parameters;
};  // < class Params

};  // < namespace mana

#endif  // < MANA_PARAMS_HPP
