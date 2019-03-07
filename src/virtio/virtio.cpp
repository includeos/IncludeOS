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
#include <kernel/events.hpp>
#include <os.hpp>
#include <kernel.hpp>
#include <hw/pci.hpp>
#include <smp>
#include <arch.hpp>
#include <assert.h>

#define VIRTIO_MSI_CONFIG_VECTOR  20
#define VIRTIO_MSI_QUEUE_VECTOR   22

Virtio::Virtio(hw::PCI_Device& dev)
  : _pcidev(dev), _virtio_device_id(dev.product_id() + 0x1040)
{
  INFO("Virtio","Attaching to  PCI addr 0x%x",dev.pci_addr());


  /** PCI Device discovery. Virtio std. §4.1.2  */

  /**
      Match vendor ID and Device ID : §4.1.2.2
  */
  assert (dev.vendor_id() == PCI::VENDOR_VIRTIO &&
          "Must be a Virtio device");
  CHECK(true, "Vendor ID is VIRTIO");

  bool _STD_ID = _virtio_device_id >= 0x1040 and _virtio_device_id < 0x107f;
  bool _LEGACY_ID = dev.product_id() >= 0x1000
    and dev.product_id() <= 0x103f;

  CHECK(_STD_ID or _LEGACY_ID, "Device ID 0x%x is in a valid range (%s)",
        dev.product_id(),
        _STD_ID ? ">= Virtio 1.0" : (_LEGACY_ID ? "Virtio LEGACY" : "INVALID"));

  assert(_STD_ID or _LEGACY_ID);

  /**
      Match Device revision ID. Virtio Std. §4.1.2.2
  */
  bool rev_id_ok = ((_LEGACY_ID and dev.rev_id() == 0) or
                    (_STD_ID and dev.rev_id() > 0));


  CHECK(rev_id_ok and version_supported(dev.rev_id()),
        "Device Revision ID (0x%x) supported", dev.rev_id());
  assert(rev_id_ok);

  // find and store capabilities
  dev.parse_capabilities();
  // find BARs etc.
  dev.probe_resources();

  // fetch I/O-base for device
  _iobase = dev.iobase();

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
  if (dev.msix_cap())
  {
    dev.init_msix();
    uint8_t msix_vectors = dev.get_msix_vectors();
    if (msix_vectors)
    {
      INFO2("[x] Device has %u MSI-X vectors", msix_vectors);
      this->current_cpu = SMP::cpu_id();

      // setup all the MSI-X vectors
      for (int i = 0; i < msix_vectors; i++)
      {
        auto irq = Events::get().subscribe(nullptr);
        dev.setup_msix_vector(current_cpu, IRQ_BASE + irq);
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
    __arch_enable_legacy_irq(irq);

    // store for later
    irqs.push_back(irq);
  }

  INFO("Virtio", "Initialization complete");
}

void Virtio::get_config(void* buf, int len)
{
  // io addr is different when MSI-X is enabled
  uint32_t ioaddr = _iobase;
  ioaddr += (has_msix()) ? VIRTIO_PCI_CONFIG_MSIX : VIRTIO_PCI_CONFIG;

  uint8_t* ptr = (uint8_t*) buf;
  for (int i = 0; i < len; i++) {
    ptr[i] = hw::inp(ioaddr + i);
  }
}


void Virtio::reset() {
  hw::outp(_iobase + VIRTIO_PCI_STATUS, 0);
}

uint8_t Virtio::get_legacy_irq()
{
  // Get legacy IRQ from PCI
  uint32_t value = _pcidev.read32(PCI::CONFIG_INTR);
  if ((value & 0xFF) != 0xFF) {
    return value & 0xFF;
  }
  return 0;
}

uint32_t Virtio::queue_size(uint16_t index) {
  hw::outpw(iobase() + VIRTIO_PCI_QUEUE_SEL, index);
  return hw::inpw(iobase() + VIRTIO_PCI_QUEUE_SIZE);
}

bool Virtio::assign_queue(uint16_t index, const void* queue_desc)
{
  hw::outpw(iobase() + VIRTIO_PCI_QUEUE_SEL, index);
  hw::outpd(iobase() + VIRTIO_PCI_QUEUE_PFN, kernel::addr_to_page((uintptr_t) queue_desc));

  if (_pcidev.has_msix())
  {
    // also update virtio MSI-X queue vector
    hw::outpw(iobase() + VIRTIO_MSI_QUEUE_VECTOR, index);
    // the programming could fail, and the reason is allocation failed on vmm
    // in which case we probably don't wanna continue anyways
    assert(hw::inpw(iobase() + VIRTIO_MSI_QUEUE_VECTOR) == index);
  }

  return hw::inpd(iobase() + VIRTIO_PCI_QUEUE_PFN) == kernel::addr_to_page((uintptr_t) queue_desc);
}

uint32_t Virtio::probe_features() {
  return hw::inpd(iobase() + VIRTIO_PCI_HOST_FEATURES);
}

void Virtio::negotiate_features(uint32_t features) {
  //_features = hw::inpd(_iobase + VIRTIO_PCI_HOST_FEATURES);
  this->_features = features;
  debug("<Virtio> Wanted features: 0x%lx \n", _features);
  hw::outpd(_iobase + VIRTIO_PCI_GUEST_FEATURES, _features);
  _features = probe_features();
  debug("<Virtio> Got features: 0x%lx \n",_features);
}

void Virtio::move_to_this_cpu()
{
  if (has_msix())
  {
    // unsubscribe IRQs on old CPU
    for (size_t i = 0; i < irqs.size(); i++)
    {
      auto& oldman = Events::get(this->current_cpu);
      oldman.unsubscribe(this->irqs[i]);
    }
    // resubscribe on the new CPU
    this->current_cpu = SMP::cpu_id();
    for (size_t i = 0; i < irqs.size(); i++)
    {
      this->irqs[i] = Events::get().subscribe(nullptr);
      _pcidev.rebalance_msix_vector(i, current_cpu, IRQ_BASE + this->irqs[i]);
    }
  }
}

void Virtio::setup_complete(bool ok)
{
  uint8_t value = hw::inp(_iobase + VIRTIO_PCI_STATUS);
  value |= ok ? VIRTIO_CONFIG_S_DRIVER_OK : VIRTIO_CONFIG_S_FAILED;
  if (!ok) {
    INFO("Virtio", "Setup failed, status: %hhx", value);
  }
  hw::outp(_iobase + VIRTIO_PCI_STATUS, value);
}
