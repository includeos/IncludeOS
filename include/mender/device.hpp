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

#ifndef MENDER_DEVICE_HPP
#define MENDER_DEVICE_HPP

#include "inventory.hpp"

namespace mender {

  class Device {
  public:
    Device(Inventory::Data_set inv)
      : inventory_(std::move(inv))
    {
      Expects(not inventory_.value("artifact_name").empty());
      Expects(not inventory_.value("device_type").empty());
    }

    Device(std::string artifact_name)
      : Device({{"artifact_name", artifact_name}, {"device_type", "includeos"}})
    {}

    Inventory& inventory()
    { return inventory_; }

    std::string& inventory(const std::string& key)
    { return inventory_[key]; }

  private:
    Inventory inventory_;
  }; // < class Device

} // < namespace mender

#endif // < MENDER_DEVICE_HPP
