// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
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

#pragma once

#ifndef MENDER_INVENTORY_HPP
#define MENDER_INVENTORY_HPP

namespace mender {

  class Inventory {
  public:
    using Data_set = std::map<std::string, std::string>;

  public:
    Inventory(std::string artifact_name, std::string device_type)
      : data_{
          {"artifact_name", std::move(artifact_name)},
          {"device_type", std::move(device_type)}
        }
    {}

    Inventory(Data_set data)
      : data_{std::move(data)}
    {}

    Data_set& data()
    { return data_; }

    inline std::string json_str() const;

    inline std::string value(const std::string& key) const;

    std::string& operator[](const std::string& key)
    { return data_[key]; }

  private:
    Data_set data_;

  }; // < class Inventory

  /* Implementation */

  std::string Inventory::json_str() const
  {
    using namespace rapidjson;
    StringBuffer buffer;
    rapidjson::Writer<StringBuffer> writer{buffer};
    writer.StartArray();

    for (auto& entry : data_) {
      writer.StartObject();
      writer.Key("name");
      writer.String(entry.first);
      writer.Key("value");
      writer.String(entry.second);
      writer.EndObject();
    }

    writer.EndArray();
    return buffer.GetString();
  }

  std::string Inventory::value(const std::string& key) const
  {
    try {
      return data_.at(key);
    }
    catch(const std::out_of_range&) {
      return {};
    }
  }

} // < namespace mender

#endif // < MENDER_INVENTORY_HPP
