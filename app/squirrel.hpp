#pragma once
#ifndef MODEL_SQUIRREL_HPP
#define MODEL_SQUIRREL_HPP

#include "json.hpp"

namespace acorn {

struct Squirrel : json::Serializable {
  size_t key;
  std::string name;
  size_t age;
  std::string occupation;

  Squirrel() : key(0) {}

  Squirrel(std::string name, size_t age, std::string occupation)
    : key(0), name(name), age(age), occupation(occupation) {}

  std::string json() const;

  friend std::ostream & operator<< (std::ostream &out, const Squirrel& t);

  virtual void serialize(rapidjson::Writer<rapidjson::StringBuffer>&) const override;
  virtual bool deserialize(const rapidjson::Document&) override;
};

std::ostream & operator<< (std::ostream &out, const Squirrel& s) {
  out << s.json();
  return out;
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

};

#endif
