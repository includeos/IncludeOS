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
#include <algorithm>
#include <rtc>

#include <json/json.hpp>

namespace acorn {

/**
 *
 */
struct Squirrel : json::Serializable {
  size_t key;

  /**
   *
   */
  Squirrel() : key(0), created_at_{RTC::now()}
  {}

  /**
   *
   */
  Squirrel(const std::string& name, const size_t age, const std::string& occupation)
    : key         {0}
    , name_       {name}
    , age_        {age}
    , occupation_ {occupation}
    , created_at_ {RTC::now()}
  {}

  /**
   *
   */
  const std::string& get_name() const noexcept
  { return name_; }

  /**
   *
   */
  void set_name(const std::string& name)
  { name_ = name; }

  /**
   *
   */
  size_t get_age() const noexcept
  { return age_; }

  /**
   *
   */
  void set_age(const size_t age) noexcept
  { age_ = age; }

  /**
   *
   */
  const std::string& get_occupation() const noexcept
  { return occupation_; }

  /**
   *
   */
  void set_occupation(const std::string& occupation)
  { occupation_ = occupation; }

  /**
   *
   */
  RTC::timestamp_t get_created_at() const noexcept
  { return created_at_; }

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
  bool is_equal(const Squirrel&) const noexcept;

  /**
   *
   */
  static bool is_equal(const Squirrel&, const Squirrel&) noexcept;

private:
  std::string      name_;
  size_t           age_;
  std::string      occupation_;
  RTC::timestamp_t created_at_;
}; //< struct Squirrel

/**--v----------- Implementation Details -----------v--**/

inline void Squirrel::serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const {
  writer.StartObject();

  writer.Key("key");
  writer.Uint(key);

  writer.Key("name");
  writer.String(name_);

  writer.Key("age");
  writer.Uint(age_);

  writer.Key("occupation");
  writer.String(occupation_);

  writer.Key("created_at");
  long hest = created_at_;
  struct tm* tt =
    gmtime (&hest);
  char datebuf[32];
  strftime(datebuf, sizeof datebuf, "%FT%TZ", tt);
  writer.String(datebuf);

  writer.EndObject();
}

inline bool Squirrel::deserialize(const rapidjson::Document& doc) {
  name_       = doc["name"].GetString();
  age_        = doc["age"].GetUint();
  occupation_ = doc["occupation"].GetString();
  return true;
}

inline std::string Squirrel::json() const {
  using namespace rapidjson;
  StringBuffer sb;
  Writer<StringBuffer> writer(sb);
  serialize(writer);
  return sb.GetString();
}

inline bool Squirrel::is_equal(const Squirrel& s) const noexcept {
  if(name_.size() not_eq s.name_.size()) {
    return false;
  }

  return std::equal(name_.begin(), name_.end(), s.name_.begin(), s.name_.end(),
    [](const auto a, const auto b) { return ::tolower(a) == ::tolower(b);
  });
}

inline bool Squirrel::is_equal(const Squirrel& s1, const Squirrel& s2) noexcept {
  return s1.is_equal(s2);
}

inline std::ostream& operator << (std::ostream& output_device, const Squirrel& s) {
  return output_device << s.json();
}

/**--^----------- Implementation Details -----------^--**/

} //< namespace acorn

#endif //< MODEL_SQUIRREL_HPP
