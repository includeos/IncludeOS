
#pragma once

#ifndef MENDER_DEVICE_HPP
#define MENDER_DEVICE_HPP

#include "inventory.hpp"

namespace mender {

  class Device {
  public:
    Device(std::string artifact_name)
      : inventory_(std::move(artifact_name), "incldueos")
    {}

    Device(Inventory::Data_set inv)
      : inventory_(std::move(inv))
    {}

    void install_update();
    void reboot();

    Inventory& inventory()
    { return inventory_; }

  private:
    Inventory inventory_;
  }; // < class Device

} // < namespace mender

#endif // < MENDER_DEVICE_HPP
