// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <virtio/virtio.hpp>
#include <kernel/irq_manager.hpp>
#include <kernel/os.hpp>
#include <kernel/syscalls.hpp>
#include <hw/pci.hpp>
#include <smp>
#include <assert.h>

#define VIRTIO_MSI_CONFIG_VECTOR  20
#define VIRTIO_MSI_QUEUE_VECTOR   22

Virtio::Virtio(hw::PCI_Device& dev)
  : _pcidev(dev), _virtio_device_id(dev.product_id() + 0x1040)
{
  INFO("Virtio","Attaching to  PCI addr 0x%x",_pcidev.pci_addr());


  /** PCI Device discovery. Virtio std. §4.1.2  */

  /**
      Match vendor ID and Device ID : §4.1.2.2
  */
  if (_pcidev.vendor_id() != hw::PCI_Device::VENDOR_VIRTIO)
    panic("This is not a Virtio device");
  CHECK(true, "Vendor ID is VIRTIO");

  bool _STD_ID = _virtio_device_id >= 0x1040 and _virtio_device_id < 0x107f;
  bool _LEGACY_ID = _pcidev.product_id() >= 0x1000
    and _pcidev.product_id() <= 0x103f;

  CHECK(_STD_ID or _LEGACY_ID, "Device ID 0x%x is in a valid range (%s)",
        _pcidev.product_id(),
        _STD_ID ? ">= Virtio 1.0" : (_LEGACY_ID ? "Virtio LEGACY" : "INVALID"));

  assert(_STD_ID or _LEGACY_ID);

  /**
      Match Device revision ID. Virtio Std. §4.1.2.2
  */
  bool rev_id_ok = ((_LEGACY_ID and _pcidev.rev_id() == 0) or
                    (_STD_ID and _pcidev.rev_id() > 0));


  CHECK(rev_id_ok and version_supported(_pcidev.rev_id()),
        "Device Revision ID (0x%x) supported", _pcidev.rev_id());

  assert(rev_id_ok); // We'll try to continue if it's newer than supported.

  // fetch I/O-base for device
  _iobase = _pcidev.iobase();

  CHECK(_iobase, "Unit has valid I/O base (0x%x)", _iobase);

  /** Device initialization. Virtio Std. v.1, sect. 3.1: */

  // 1. Reset device
  reset();
  INFO2("[*] Reset device");

  // 2. Set ACKNOWLEGE status bit, and
  // 3. Set DRIVER status bit

  hw::outp(_iobase + VIRTIO_PCI_STATUS,
           hw::inp(_iobase + VIRTIO_PCI_STATUS) |
           VIRTIO_CONFIG_S_ACKNOWLEDGE |
           VIRTIO_CONFIG_S_DRIVER);


  // THE REMAINING STEPS MUST BE DONE IN A SUBCLASS
  // 4. Negotiate features (Read, write, read)
  //    => In the subclass (i.e. Only the Nic driver knows if it wants a mac)
  // 5. @todo IF >= Virtio 1.0, set FEATURES_OK status bit
  // 6. @todo IF >= Virtio 1.0, Re-read Device Status to ensure features are OK
  // 7. Device specifig setup.

  // Where the standard isn't clear, we'll do our best to separate work
  // between this class and subclasses.

  // initialize MSI-X if available
  if (_pcidev.has_msix())
  {
    uint8_t msix_vectors = _pcidev.get_msix_vectors();
    if (msix_vectors)
    {
      INFO2("[x] Device has %u MSI-X vectors", msix_vectors);
      this->current_cpu = SMP::cpu_id();

      // setup all the MSI-X vectors
      for (int i = 0; i < msix_vectors; i++)
      {
        auto irq = IRQ_manager::get().get_free_irq();
        _pcidev.setup_msix_vector(current_cpu, IRQ_BASE + irq);
        IRQ_manager::get().subscribe(irq, nullptr);
        // store IRQ for later
        this->irqs.push_back(irq);
      }
    }
    else
      INFO2("[ ] No MSI-X vectors");
  } else {
    INFO2("[ ] No MSI-X vectors");
  }

  // use legacy if msix was not enabled
  if (has_msix() == false)
  {
    // Fetch IRQ from PCI resource
    auto irq = get_legacy_irq();
    CHECKSERT(irq, "Unit has legacy IRQ %u", irq);

   // create IO APIC entry for legacy interrupt
    extern void __arch_enable_legacy_irq(uint8_t);
    __arch_enable_legacy_irq(irq);

    // store for later
    irqs.push_back(irq);
  }

  INFO("Virtio", "Initialization complete");

  // It would be nice if we new that all queues were the same size.
  // Then we could pass this size on to the device-specific constructor
  // But, it seems there aren't any guarantees in the standard.

  // @note this is "the Legacy interface" according to Virtio std. 4.1.4.8.
  // uint32_t queue_size = hw::inpd(_iobase + 0x0C);
}

void Virtio::get_config(void* buf, int len){
  // io addr is different when MSI-X is enabled
  uint32_t ioaddr = _iobase;
  ioaddr += (has_msix()) ? VIRTIO_PCI_CONFIG_MSIX : VIRTIO_PCI_CONFIG;

  uint8_t* ptr = (uint8_t*) buf;
  for (int i = 0; i < len; i++)
    ptr[i] = hw::inp(ioaddr + i);
}


void Virtio::reset() {
  hw::outp(_iobase + VIRTIO_PCI_STATUS, 0);
}

uint8_t Virtio::get_legacy_irq()
{
  // Get legacy IRQ from PCI
  uint32_t value = _pcidev.read_dword(PCI::CONFIG_INTR);
  if ((value & 0xFF) > 0 && (value & 0xFF) < 32){
    return value & 0xFF;
  }
  return 0;
}

uint32_t Virtio::queue_size(uint16_t index){
  hw::outpw(iobase() + VIRTIO_PCI_QUEUE_SEL, index);
  return hw::inpw(iobase() + VIRTIO_PCI_QUEUE_SIZE);
}

#define BTOP(x) ((unsigned long)(x) >> PAGESHIFT)
bool Virtio::assign_queue(uint16_t index, uint32_t queue_desc){
  hw::outpw(iobase() + VIRTIO_PCI_QUEUE_SEL, index);
  hw::outpd(iobase() + VIRTIO_PCI_QUEUE_PFN, OS::page_nr_from_addr(queue_desc));

  if (_pcidev.has_msix())
  {
    // also update virtio MSI-X queue vector
    hw::outpw(iobase() + VIRTIO_MSI_QUEUE_VECTOR, index);
    // the programming could fail, and the reason is allocation failed on vmm
    // in which case we probably don't wanna continue anyways
    assert(hw::inpw(iobase() + VIRTIO_MSI_QUEUE_VECTOR) == index);
  }

  return hw::inpd(iobase() + VIRTIO_PCI_QUEUE_PFN) == OS::page_nr_from_addr(queue_desc);
}

uint32_t Virtio::probe_features(){
  return hw::inpd(_iobase + VIRTIO_PCI_HOST_FEATURES);
}

void Virtio::negotiate_features(uint32_t features){
  _features = hw::inpd(_iobase + VIRTIO_PCI_HOST_FEATURES);
  //_features &= features; //SanOS just adds features
  _features = features;
  debug("<Virtio> Wanted features: 0x%lx \n",_features);
  hw::outpd(_iobase + VIRTIO_PCI_GUEST_FEATURES, _features);
  _features = probe_features();
  debug("<Virtio> Got features: 0x%lx \n",_features);

}

void Virtio::setup_complete(bool ok){
  uint8_t status = ok ? VIRTIO_CONFIG_S_DRIVER_OK : VIRTIO_CONFIG_S_FAILED;
  debug("<VIRTIO> status: %i ",status);
  hw::outp(_iobase + VIRTIO_PCI_STATUS, hw::inp(_iobase + VIRTIO_PCI_STATUS) | status);
}

void Virtio::move_to_this_cpu()
{
  if (has_msix())
  {
    // unsubscribe IRQs on old CPU
    for (size_t i = 0; i < irqs.size(); i++)
    {
      auto& oldman = IRQ_manager::get(this->current_cpu);
      oldman.unsubscribe(this->irqs[i]);
    }
    // resubscribe on the new CPU
    this->current_cpu = SMP::cpu_id();
    for (size_t i = 0; i < irqs.size(); i++)
    {
      this->irqs[i] = IRQ_manager::get().get_free_irq();
      _pcidev.rebalance_msix_vector(i, current_cpu, IRQ_BASE + this->irqs[i]);
      IRQ_manager::get().subscribe(this->irqs[i], nullptr);
    }
  }
}
