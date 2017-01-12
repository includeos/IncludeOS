
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
    using namespace nlohmann;
    json list = json::array();
    for(auto& entry : data_)
      list.push_back({{"name", entry.first}, {"value", entry.second}});
    return list.dump();
  }

  std::string Inventory::value(const std::string& key) const
  {
    try {
      return data_.at(key);
    }
    catch(std::out_of_range) {
      return {};
    }
  }

} // < namespace mender

#endif // < MENDER_INVENTORY_HPP
