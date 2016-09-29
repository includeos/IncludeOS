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

// User med en key (tilsvarer cookie-id)
//
// Brukeridentifisering
//
// Starte med:
// Sette en favorittdyr = hest: kommer opp hest på siden

// Se squirrel.hpp

#pragma once
#ifndef MODEL_USER_HPP
#define MODEL_USER_HPP

//#include <locale>
//#include <algorithm>

#include <json/json.hpp>

namespace acorn {

/**
 *
 */
struct User : json::Serializable {
  size_t key;

  /**
   *
   */
  User() : key{0} {}

  // More constructors here

  /**
   *
   */
  std::string json() const;

  /**
   *
   */
  virtual void serialize(rapidjson::Writer<rapidjson::StringBuffer>&) const override;

  /**
   *
   */
  virtual bool deserialize(const rapidjson::Document&) override;

  /**
   *
   */
  bool is_equal(const User&) const;

  /**
   *
   */
  static bool is_equal(const User&, const User&);

};

/**--v----------- Implementation Details -----------v--**/

inline void User::serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const {
  writer.StartObject();

  writer.Key("key");
  writer.Uint(key);

  // Write more variables here

  writer.EndObject();
}

inline bool User::deserialize(const rapidjson::Document& doc) {
  key = doc["key"].GetUint();

  // set more variables here

  return true;
}

inline std::string User::json() const {
  using namespace rapidjson;
  StringBuffer sb;
  Writer<StringBuffer> writer(sb);
  serialize(writer);
  return sb.GetString();
}

inline bool User::is_equal(const User& u) const {
  return key == u.key;
}

inline bool User::is_equal(const User& u1, const User& u2) {
  return u1.is_equal(u2);
}

inline std::ostream& operator << (std::ostream& output_device, const User& u) {
  return output_device << u.json();
}

/**--^----------- Implementation Details -----------^--**/

} //< namespace acorn

#endif //< MODEL_USER_HPP
