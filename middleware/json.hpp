#ifndef ATTRIBUTE_JSON_HPP
#define ATTRIBUTE_JSON_HPP

#include "attribute.hpp"

class Json : public server::Attribute {
private:
  using KeyValueMap = std::map<std::string, std::string>;

public:

  static server::AttrType type(server::AttrType t = 0) {
    static server::AttrType type;
    if(t && !type) type = t;
    return type;
  }

  KeyValueMap& members()
  { return members_; }

private:
  std::map<std::string, std::string> members_;

};

#endif
