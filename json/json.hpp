// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
