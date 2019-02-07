// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2019 IncludeOS AS, Oslo, Norway
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

#include <string>

namespace hw {

  class Device {
  public:
    enum class Type
    {
      Block,
      Nic
    };

    virtual void deactivate() = 0;
    virtual void flush() {} // optional

    virtual Type device_type() const noexcept = 0;
    virtual std::string device_name() const = 0;

    virtual std::string to_string() const
    { return to_string(device_type()) +  " " + device_name(); }

    static std::string to_string(const Type type)
    {
      switch(type)
      {
        case Type::Block: return "Block device";
        case Type::Nic:   return "NIC";
        default:          return "Unknown device";
      }
    }
  };

}
