#include "stringer_j.hpp"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

namespace acorn {

struct Squirrel {
  size_t key;
  std::string name;
  size_t age;
  std::string occupation;

  Squirrel() : key(0) {}

  Squirrel(std::string name, size_t age, std::string occupation)
    : key(0), name(name), age(age), occupation(occupation) {}

  std::string json() const;

  friend std::ostream & operator<< (std::ostream &out, const Squirrel& t);
};

std::ostream & operator<< (std::ostream &out, const Squirrel& s) {
  out << s.json();
  return out;
}

std::string Squirrel::json() const {
  using namespace rapidjson;
  StringBuffer s;
  Writer<StringBuffer> writer(s);
  writer.StartObject();
  // name
  writer.Key("name");
  writer.String(name.c_str());
  // age
  writer.Key("age");
  writer.Uint(age);
  // occupation
  writer.Key("occupation");
  writer.String(occupation.c_str());

  writer.EndObject();
  return s.GetString();
}

};
