
#pragma once

#ifndef MENDER_DEVICE_HPP
#define MENDER_DEVICE_HPP

#include "inventory.hpp"

namespace mender {

  class Device {
  public:
    Device(void* update_loc, Inventory::Data_set inv)
      : update_loc_(update_loc),
        inventory_(std::move(inv))
    {}

    Device(void* update_loc, std::string artifact_name)
      : Device(update_loc, {{"artifact_name", artifact_name}, {"device_type", "includeos"}})
    {}

    void* update_loc() const
    { return update_loc_; }

    Inventory& inventory()
    { return inventory_; }

    std::string& inventory(const std::string& key)
    { return inventory_[key]; }

  private:
    void* update_loc_ = nullptr;
    Inventory inventory_;
  }; // < class Device

} // < namespace mender

#endif // < MENDER_DEVICE_HPP
