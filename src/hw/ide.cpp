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

/**
 *  Intel IDE Controller datasheet at :
 *  ftp://download.intel.com/design/intarch/datashts/29055002.pdf
 */

#include <hw/ide.hpp>

#include <kernel/irq_manager.hpp>
#include <kernel/syscalls.hpp>
#include <hw/ioport.hpp>

#define IDE_DATA        0x1F0
#define IDE_SECCNT      0x1F2
#define IDE_BLKLO       0x1F3
#define IDE_BLKMID      0x1F4
#define IDE_BLKHI       0x1F5
#define IDE_DRV         0x1F6
#define IDE_CMD         0x1F7
#define IDE_STATUS      IDE_CMD

#define IDE_CMD_READ     0x20
#define IDE_CMD_WRITE    0x30
#define IDE_CMD_IDENTIFY 0xEC

#define IDE_DRQ      (1 << 3)
#define IDE_DRDY     (1 << 6)
#define IDE_BUSY     (1 << 7)

#define IDE_CTRL_IRQ 0x3F6
#define IDE_IRQN     14
#define IDE_BLKSZ    512

#define IDE_VENDOR_ID   PCI_Device::VENDOR_INTEL
#define IDE_PRODUCT_ID  0x7010

#define IDE_TIMEOUT 2048

namespace hw {

  IDE::IDE(hw::PCI_Device& pcidev, selector_t sel) :
    _pcidev {pcidev},
    _drive  {(uint8_t)sel},
    _iobase {0U},
    _nb_blk {0U}
{
  INFO("IDE","VENDOR_ID : 0x%x, PRODUCT_ID : 0x%x", _pcidev.vendor_id(), _pcidev.product_id());
  INFO("IDE","Attaching to  PCI addr 0x%x",_pcidev.pci_addr());

  /** PCI device checking */
  if (_pcidev.vendor_id() not_eq IDE_VENDOR_ID) {
    panic("This is not an Intel device");
  }
  CHECK(true, "Vendor ID is INTEL");

  if (_pcidev.product_id() not_eq IDE_PRODUCT_ID) {
    panic("This is not an IDE Controller");
  }
  CHECK(true, "Product ID is IDE Controller");

  /** Probe PCI resources and fetch I/O-base for device */
  _pcidev.probe_resources();
  _iobase = _pcidev.iobase();
  CHECK(_iobase, "Unit has valid I/O base (0x%x)", _iobase);

  /** IRQ initialization */
  CHECK(IDE_IRQN, "Unit has IRQ %i", IDE_IRQN);
  enable_irq_handler();
  INFO("IDE", "Enabling IRQ handler");

  /** IDE device initialization */
  set_irq_mode(false);
  set_drive(0xA0 | _drive);
  set_nbsectors(0U);
  set_blocknum(0U);
  set_command(IDE_CMD_IDENTIFY);

  if (not inb(IDE_STATUS)) {
    panic("Device not found");
  }
  CHECK(true, "IDE device found");
  wait_status_flags(IDE_DRDY, false);

  uint16_t buffer[256];
  for (int i {0}; i < 256; ++i) {
    buffer[i] = inw(IDE_DATA);
  }

  _nb_blk = (buffer[61] << 16) | buffer[60];

  INFO("IDE", "Initialization complete");
}

  struct ide_irq {
    ide_irq(uint8_t* buff, IDE::on_read_func call)
      : buffer(buff)
      , callback(call)
    {}

    uint8_t*          buffer;   // Reading buffer
    IDE::on_read_func callback; // IRQ callback
  };

  static int                       _nb_irqs = 0;                // Number of IRQs that we expect
  static IDE::on_read_func         _current_callback = nullptr; // Callback for the current irq
  static std::list<struct ide_irq> _ide_irqs;                   // IRQ queue

  void IDE::read(block_t blk, on_read_func callback) {
    if (blk >= _nb_blk) {
      // avoid reading past the disk boundaries
      callback(buffer_t());
      return;
    }

    set_irq_mode(true);
    set_drive(0xE0 | _drive | ((blk >> 24) & 0x0F));
    set_nbsectors(1);
    set_blocknum(blk);
    set_command(IDE_CMD_READ);

    _current_callback = callback;
    _nb_irqs = 1;
  }

  void IDE::read(block_t blk, size_t count, on_read_func callback)
  {
    if (blk + count >= _nb_blk) {
      // avoid reading past the disk boundaries
      callback(buffer_t());
      return;
    }

    set_irq_mode(true);
    set_drive(0xE0 | _drive | ((blk >> 24) & 0x0F));
    set_nbsectors(count);
    set_blocknum(blk);
    set_command(IDE_CMD_READ);

    _current_callback = callback;
    _nb_irqs = count;
  }

  IDE::buffer_t IDE::read_sync(block_t blk)
  {
    if (blk >= _nb_blk) {
      // avoid reading past the disk boundaries
      return buffer_t();
    }

    set_irq_mode(false);
    set_drive(0xE0 | _drive | ((blk >> 24) & 0x0F));
    set_nbsectors(1);
    set_blocknum(blk);
    set_command(IDE_CMD_READ);

    auto* buffer = new uint8_t[block_size()];

    wait_status_flags(IDE_DRDY, false);

    uint16_t* wptr = (uint16_t*) buffer;
    uint16_t* wend = (uint16_t*)&buffer[block_size()];
    while (wptr < wend)
      *(wptr++) = inw(IDE_DATA);

    // return a shared_ptr wrapper for the buffer
    return buffer_t(buffer, std::default_delete<uint8_t[]>());
  }
  IDE::buffer_t IDE::read_sync(block_t blk, size_t cnt) {
    (void) blk;
    (void) cnt;
    // not yet implemented
    return buffer_t();
  }

  void IDE::wait_status_busy() noexcept {
    uint8_t ret;
    while (((ret = inb(IDE_STATUS)) & IDE_BUSY) == IDE_BUSY);
  }

  void IDE::wait_status_flags(const int flags, const bool set) noexcept {
    wait_status_busy();

    auto ret = inb(IDE_STATUS);

    for (int i {IDE_TIMEOUT}; i; --i) {
      if (set) {
        if ((ret & flags) == flags)
          break;
      } else {
        if ((ret & flags) not_eq flags)
          break;
      }

      ret = inb(IDE_STATUS);
    }
  }

  void IDE::set_drive(const uint8_t drive) const noexcept {
    wait_status_flags(IDE_DRQ, true);
    outb(IDE_DRV, drive);
  }

  void IDE::set_nbsectors(const uint8_t cnt) const noexcept {
    wait_status_flags(IDE_DRQ, true);
    outb(IDE_SECCNT, cnt);
  }

  void IDE::set_blocknum(block_t blk) const noexcept {
    wait_status_flags(IDE_DRQ, true);
    outb(IDE_BLKLO, blk & 0xFF);

    wait_status_flags(IDE_DRQ, true);
    outb(IDE_BLKMID, (blk & 0xFF00) >> 8);

    wait_status_flags(IDE_DRQ, true);
    outb(IDE_BLKHI, (blk & 0xFF0000) >> 16);
  }

  void IDE::set_command(const uint16_t command) const noexcept {
    wait_status_flags(IDE_DRDY, false);
    outb(IDE_CMD, command);
  }

  void IDE::set_irq_mode(const bool on) noexcept {
    wait_status_flags(IDE_DRDY, false);
    outb(IDE_CTRL_IRQ, on ? 0 : 1);
  }

  extern "C"
  void ide_irq_handler() {
    if (!_nb_irqs || _current_callback == nullptr) {
      IDE::set_irq_mode(false);
      return;
    }

    uint8_t* buffer = new uint8_t[IDE_BLKSZ];

    IDE::wait_status_flags(IDE_DRDY, false);

    uint16_t* wptr = (uint16_t*) buffer;

    for (IDE::block_t i = 0; i < IDE_BLKSZ / sizeof (uint16_t); ++i)
      wptr[i] = inw(IDE_DATA);

    _ide_irqs.push_back(ide_irq(buffer, _current_callback));
    _nb_irqs--;
  }

  extern "C" void ide_irq_entry();

  void IDE::callback_wrapper()
  {
    IDE::on_read_func callback = _ide_irqs.front().callback;
    callback(IDE::buffer_t(_ide_irqs.front().buffer, std::default_delete<uint8_t[]>()));
    _ide_irqs.pop_front();
  }

  void IDE::enable_irq_handler() {
    auto del(delegate<void()>::from<IDE, &IDE::callback_wrapper>(this));
    IRQ_manager::get().subscribe(IDE_IRQN, del);
    //IRQ_manager::cpu(0).set_irq_handler(IDE_IRQN + 32, ide_irq_entry);
  }

} //< namespace hw
