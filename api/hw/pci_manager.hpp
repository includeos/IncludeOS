
#ifndef KERNEL_PCI_MANAGER_HPP
#define KERNEL_PCI_MANAGER_HPP

#include <cstdint>
#include <hw/nic.hpp>
#include <hw/block_device.hpp>
#include <hw/pci_device.hpp>

namespace hw {

class PCI_manager {
public:
  // a <...> driver is constructed from a PCI device,
  //   and returns a unique_ptr to itself
  using NIC_driver = delegate< std::unique_ptr<hw::Nic> (PCI_Device&, uint16_t) >;
  using Device_vector = std::vector<const hw::PCI_Device*>;
  static void register_nic(uint16_t, uint16_t, NIC_driver);

  using BLK_driver = delegate< std::unique_ptr<hw::Block_device> (PCI_Device&) >;
  static void register_blk(uint16_t, uint16_t, BLK_driver);

  static void init();
  static void init_devices(uint8_t classcode);
  static  Device_vector devices();

private:
  static void scan_bus(int bus);
};

}

#endif //< KERNEL_PCI_MANAGER_HPP
