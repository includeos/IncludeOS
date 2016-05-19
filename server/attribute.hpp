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


}; // < namespace server

#endif
