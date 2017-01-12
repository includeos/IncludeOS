
#pragma once

#ifndef MENDER_DEVICE_HPP
#define MENDER_DEVICE_HPP

#include "inventory.hpp"

namespace mender {

  class Device {
  public:
    Device(void* update_loc, std::string artifact_name)
      : update_loc_(update_loc),
        inventory_(std::move(artifact_name), "includeos")
    {}

    Device(void* update_loc, Inventory::Data_set inv)
      : update_loc_(update_loc),
        inventory_(std::move(inv))
    {}

    void install_update();
    void reboot();

    Inventory& inventory()
    { return inventory_; }

  private:
    void* update_loc_ = nullptr;
    Inventory inventory_;
  }; // < class Device

} // < namespace mender

#endif // < MENDER_DEVICE_HPP
