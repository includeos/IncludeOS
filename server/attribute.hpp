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
