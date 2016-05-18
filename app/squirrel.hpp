#include "stringer_j.hpp"

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
  stringerj::StringerJ json;
  json.add("name", name).add("age", age).add("occupation", occupation);
  return json.str();
}

};
