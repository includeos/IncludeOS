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

#define IDE_MASTER  0x00
#define IDE_SLAVE   0x10

#define IDE_DRQ      (1 << 3)
#define IDE_DRDY     (1 << 6)
#define IDE_BUSY     (1 << 7)

#define IDE_VENDOR_ID   PCI_Device::VENDOR_INTEL
#define IDE_PRODUCT_ID  0x7010

#define IDE_TIMEOUT 2048

namespace hw {

IDE::IDE(hw::PCI_Device& pcidev) noexcept:
  _pcidev {pcidev},
  _drive  {IDE_MASTER},
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

  /** IDE device initialization */
  set_drive(_drive);
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

void IDE::read_sector(block_t blk, on_read_func reader) {
  if (blk >= _nb_blk) {
    // avoid reading past the disk boundaries
    reader(nullptr);
    return;
  }

  set_drive(0xE0 | (_drive << 4) | ((blk >> 24) & 0x0F));
  set_nbsectors(1);
  set_blocknum(blk);
  set_command(IDE_CMD_READ);

  auto* buffer = new uint8_t[block_size()];

  wait_status_flags(IDE_DRDY, false);
  
  uint16_t* wptr = (uint16_t*) buffer;
  
  for (block_t i = 0; i < block_size() / sizeof (uint16_t); ++i)
    wptr[i] = inw(IDE_DATA);
  
  // return a shared_ptr wrapper for the buffer
  reader( buffer_t(buffer, std::default_delete<uint8_t[]>()) );
}

void IDE::read_sectors(block_t, block_t, on_read_func callback)
{
  // not implemented yet
  callback( buffer_t() );
}

void IDE::wait_status_busy() const noexcept {
  uint8_t ret;
  while (((ret = inb(IDE_STATUS)) & IDE_BUSY) == IDE_BUSY);
}

void IDE::wait_status_flags(const int flags, const bool set) const noexcept {
  wait_status_busy();

  auto ret = inb(IDE_STATUS);

  for (int i {IDE_TIMEOUT}; i; --i) {
    if (set) {
      if ((ret & flags) not_eq flags)
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

} //< namespace hw
