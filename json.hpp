#ifndef JSON_HPP
#define JSON_HPP

#define RAPIDJSON_HAS_STDSTRING 1
#define RAPIDJSON_THROWPARSEEXCEPTION 1

class AssertException : public std::logic_error {
public:
  AssertException(const char* w) : std::logic_error(w) {}
};
#define RAPIDJSON_ASSERT(x) if (!(x)) throw AssertException(RAPIDJSON_STRINGIFY(x))

#include "attribute.hpp"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"


namespace json {

struct Serializable {
  virtual void serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const = 0;
  virtual bool deserialize(const rapidjson::Document& doc) = 0;
};


class JsonDoc : public server::Attribute {
public:
  rapidjson::Document& doc()
  { return document_; }

private:
  rapidjson::Document document_;

};

}; // < namespace json

#endif
