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
#ifndef MODEL_SQUIRREL_HPP
#define MODEL_SQUIRREL_HPP

#include <locale>

#include "json.hpp"

namespace acorn {

/**
 *
 */
struct Squirrel : json::Serializable {
  size_t key;
  std::string name;
  size_t age;
  std::string occupation;

  Squirrel() : key(0) {}

  /**
   *
   */
  Squirrel(std::string name, size_t age, std::string occupation)
    : key(0), name(name), age(age), occupation(occupation) {}

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
  bool is_equal(const Squirrel&) const;

  /**
   *
   */
  static bool is_equal(const Squirrel&, const Squirrel&);
};

inline std::ostream& operator << (std::ostream& output_device, const Squirrel& s) {
  return output_device.json();
}

void Squirrel::serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const {
  writer.StartObject();

  writer.Key("key");
  writer.Uint(key);

  writer.Key("name");
  writer.String(name);

  writer.Key("age");
  writer.Uint(age);

  writer.Key("occupation");
  writer.String(occupation);

  writer.EndObject();
}

bool Squirrel::deserialize(const rapidjson::Document& doc) {
  name = doc["name"].GetString();
  age = doc["age"].GetUint();
  occupation = doc["occupation"].GetString();
  return true;
}

std::string Squirrel::json() const {
  using namespace rapidjson;
  StringBuffer sb;
  Writer<StringBuffer> writer(sb);
  serialize(writer);
  return sb.GetString();
}

bool Squirrel::is_equal(const Squirrel& s) const {
  if(name.size() != s.name.size())
    return false;
  for(size_t i = 0; i < name.size(); i++) {
    if(std::tolower(name[i]) != std::tolower(s.name[i]))
      return false;
  }
  return true;
}

bool Squirrel::is_equal(const Squirrel& s1, const Squirrel& s2) {
  return s1.is_equal(s2);
}

} // < namespace acorn

#endif
