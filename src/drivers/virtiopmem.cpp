#include "virtiopmem.hpp"
#include <hw/pci_manager.hpp>
#include <info>
#include <os>

VirtioPMEM_device::VirtioPMEM_device(hw::PCI_Device& d) : Virtio_control(d),
_req(*this, 0, true) {
  static int id_count = 0;
  _id = id_count++;
  _config = reinterpret_cast<virtio_pmem_config*>(specific_cfg());

  set_driver_ok_bit();
  INFO("VirtioPMEM", "Initializing VirtioPMEM was a success!");
}

int VirtioPMEM_device::id() const noexcept {
  return _id;
}

/** Method returns the name of the device */
std::string VirtioPMEM_device::device_name() const {
  return "VirtioPMEM" + std::to_string(_id);
}

/** Factory method used to create VirtioFS driver object */
std::unique_ptr<hw::DAX_device> VirtioPMEM_device::new_instance(hw::PCI_Device& d) {
  return std::make_unique<VirtioPMEM_device>(d);
}

void VirtioPMEM_device::flush() {
  INFO("VirtioPMEM", "Executing flush");

  virtio_pmem_req req {VIRTIO_PMEM_REQ_TYPE_FLUSH};
  virtio_pmem_res res { 0 };

  VirtTokens flush_tokens;
  flush_tokens.reserve(2);

  flush_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    reinterpret_cast<uint8_t*>(&req),
    sizeof(virtio_pmem_req)
  );
  flush_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(&res),
    sizeof(virtio_pmem_res)
  );

  _req.enqueue(flush_tokens);
  _req.kick();
  while(_req.has_processed_used());
  _req.dequeue();

  if(res.ret == 0) {
    INFO("VirtioPMEM", "Flushing was a success!");
  } else {
    INFO("VirtioPMEM", "Flushing was a failure:(");
  }
}

void VirtioPMEM_device::deactivate() {
  INFO("VirtioPMEM", "Deactivating device");
  flush();
  _req.deactivate_split_queue();
  deactivate_virtio_control();
}

__attribute__((constructor))
void autoreg_virtiopmem() {
  hw::PCI_manager::register_dax(PCI::VENDOR_VIRTIO, 0x105b, &VirtioPMEM_device::new_instance);
}
