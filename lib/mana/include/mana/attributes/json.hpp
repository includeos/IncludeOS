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

#ifndef MANA_ATTRIBUTES_JSON_HPP
#define MANA_ATTRIBUTES_JSON_HPP

#ifndef RAPIDJSON_HAS_STDSTRING
  #define RAPIDJSON_HAS_STDSTRING 1
#endif

#ifndef RAPIDJSON_THROWPARSEEXCEPTION
  #define RAPIDJSON_THROWPARSEEXCEPTION 1
#endif

#include <mana/attribute.hpp>
#include <rapidjson/writer.h>
#include <rapidjson/document.h>


namespace mana {

  struct Serializable {
    virtual void serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const = 0;

    virtual bool deserialize(const rapidjson::Document& doc) = 0;
  }; //< struct Serializable

  namespace attribute {

    class Json_doc : public mana::Attribute {
    public:

      rapidjson::Document& doc()
      { return document_; }

    private:
      rapidjson::Document document_;

    }; //< class Json_doc

  } //< namespace attribute
} //< namespace mana

#endif //< JSON_JSON_HPP
