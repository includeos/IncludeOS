#ifndef ATTRIBUTE_JSON_HPP
#define ATTRIBUTE_JSON_HPP

#include "attribute.hpp"

class Json : public server::Attribute {
private:
  using KeyValueMap = std::map<std::string, std::string>;

public:

  KeyValueMap& members()
  { return members_; }

private:
  std::map<std::string, std::string> members_;

};

#endif
