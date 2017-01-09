
#pragma once

#ifndef MENDER_INVENTORY_HPP
#define MENDER_INVENTORY_HPP

namespace mender {

  class Inventory {
    using Data_set = std::map<std::string, std::string>;
  public:
    Inventory(std::string device_type)
      : device_type_(std::move(device_type))
    {}

    Inventory()
      : Inventory("includeos")
    {}

    std::string json_str()
    {
      using namespace nlohmann;
      json list = json::array();
      list.push_back({{"name", "device_type"}, {"value", device_type_}});
      return list.dump();
    }
  private:
    std::string device_type_;
  }; // < class Device

} // < namespace mender

#endif // < MENDER_INVENTORY_HPP
