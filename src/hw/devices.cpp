

#include <hw/devices.hpp>
#include <virtio/virtionet.hpp>
#include <virtio/block.hpp>

using namespace hw;

using NicDevices = std::vector<std::unique_ptr<hw::Nic>>;
static NicDevices nic_devices_;

static std::vector<std::unique_ptr<hw::Disk<VirtioBlk>>> disk_devices_;

hw::Nic& Devices::nic(const int N) {
  return *(nic_devices_.at(N));
}

hw::Disk<VirtioBlk>& Devices::disk(const int N) {
  return *(disk_devices_.at(N));
}

/*__attribute__((weak))
bool register_virtionet_driver(NicDevices&, PCI_Device&)
{
  INFO("Devices", "NIC VirtioNet Driver not included");
  return false;
}*/

bool register_virtionet_driver(NicDevices& nics, PCI_Device& d)
{
  nics.emplace_back(std::make_unique<VirtioNet>(d));
  return true;
}

void Devices::register_device(std::unique_ptr<Nic> nic) {
  nic_devices_.emplace_back(std::move(nic));
  INFO("Devices", "NIC [%u] registered (%s)", nic_devices_.size()-1, nic_devices_.back()->name());
}
