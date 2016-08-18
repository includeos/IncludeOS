

#include <hw/devices.hpp>
#include <virtio/virtionet.hpp>
#include <virtio/block.hpp>

using namespace hw;

static std::vector<std::unique_ptr<hw::Nic>> nic_devices_;

static std::vector<std::unique_ptr<hw::Disk<VirtioBlk>>> disk_devices_;

hw::Nic& Devices::nic(const int N) {
  return *(nic_devices_.at(N));
}

hw::Disk<VirtioBlk>& Devices::disk(const int N) {
  return *(disk_devices_.at(N));
}

void Devices::register_device(PCI_Device& d) {

  switch(d.classcode())
  {
    case PCI::NIC:
    {
      // if VirtioNet
      if(d.vendor_id() == PCI_Device::VENDOR_VIRTIO) {
        nic_devices_.emplace_back(std::make_unique<VirtioNet>(d));
        INFO("Devices", "NIC [%u] registered (%s)", nic_devices_.size()-1, nic_devices_.back()->name());
      }
      break;
    }

    case PCI::STORAGE:
    {
      //if VirtioBlk
      if(d.vendor_id() == PCI_Device::VENDOR_VIRTIO) {
        disk_devices_.emplace_back(std::make_unique<Disk<VirtioBlk>>(d));
        INFO("Devices", "Disk [%u] registered (%s)", disk_devices_.size()-1, disk_devices_.back()->name());
      }
      break;
    }
    case PCI::BRIDGE: break;

    default:
    {
      INFO("Devices", "%#x", d.classcode());
    }
  }
}
