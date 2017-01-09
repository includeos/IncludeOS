
#pragma once

#ifndef MENDER_DEVICE_HPP
#define MENDER_DEVICE_HPP

#include "inventory.hpp"

namespace mender {

  class Device {
  public:
    void install_update();
    void reboot();

    Inventory& inventory()
    { return inventory_; }

  private:
    Inventory inventory_;
  }; // < class Device

} // < namespace mender

#endif // < MENDER_DEVICE_HPP
