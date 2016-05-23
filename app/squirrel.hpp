#include "json_writer.hpp"

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

  template <typename Writer>
  void serialize(Writer& writer) const;
};

std::ostream & operator<< (std::ostream &out, const Squirrel& s) {
  out << s.json();
  return out;
}

template <typename Writer>
void Squirrel::serialize(Writer& writer) const {
  writer.start_object();
  writer.add("name", name);
  writer.add("age", age);
  writer.add("occupation", occupation);
  writer.end_object();
}

std::string Squirrel::json() const {
  using namespace rapidjson;
  StringBuffer sb;
  JSON_Writer<StringBuffer> writer(sb);
  serialize(writer);
  return sb.GetString();
}

};
