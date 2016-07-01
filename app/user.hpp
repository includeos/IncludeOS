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

#include "json.hpp"
//#include <locale>

namespace acorn {

// struct or class?

struct User : json::Serializable {
  size_t key;

  User(size_t k) : key{k} {}

  // More constructors here

  std::string json() const;

  friend std::ostream & operator<< (std::ostream &out, const User& u);

  virtual void serialize(rapidjson::Writer<rapidjson::StringBuffer>&) const override;
  virtual bool deserialize(const rapidjson::Document&) override;

  bool is_equal(const User&) const;

  static bool is_equal(const User&, const User&);

};

// -------------------- Implementation ------------------------

std::ostream & operator<< (std::ostream &out, const User& u) {
  out << u.json();
  return out;
}

void User::serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const {
  writer.StartObject();

  writer.Key("key");
  writer.Uint(key);

  // Write more variables here

  writer.EndObject();
}

bool User::deserialize(const rapidjson::Document& doc) {
  key = doc["key"].GetUint();

  // set more variables here

  return true;
}

std::string User::json() const {
  using namespace rapidjson;
  StringBuffer sb;
  Writer<StringBuffer> writer(sb);
  serialize(writer);
  return sb.GetString();
}

bool User::is_equal(const User& u) const {
  if(key == u.key)
    return false;

  return true;
}

bool User::is_equal(const User& u1, const User& u2) {
  return u1.is_equal(u2);
}

} // < namespace acorn

#endif
