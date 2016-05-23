#ifndef ATTRIBUTE_JSON_HPP
#define ATTRIBUTE_JSON_HPP
#define RAPIDJSON_HAS_STDSTRING 1
#define RAPIDJSON_THROWPARSEEXCEPTION 1
class AssertException : public std::logic_error {
public:
    AssertException(const char* w) : std::logic_error(w) {}
};
#define RAPIDJSON_ASSERT(x) if (!(x)) throw AssertException(RAPIDJSON_STRINGIFY(x))

#include "attribute.hpp"
#include "rapidjson/document.h"

class Json : public server::Attribute {
public:
  rapidjson::Document& doc()
  { return document_; }

private:
  rapidjson::Document document_;

};

#endif
