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

#include "ide.hpp"
#include <hw/pci_device.hpp>
#include <hw/ioport.hpp>
#include <kernel/events.hpp>
#include <fs/common.hpp>
#include <arch.hpp>
#include <inttypes.h>

//#define IDE_DEBUG
#ifdef IDE_DEBUG
#define IDBG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define IDBG(fmt, ...) /* fmt */
#endif


#define IDE_DATA        0x1F0
#define IDE_SECCNT      0x1F2
#define IDE_LBA0        0x1F3
#define IDE_LBA1        0x1F4
#define IDE_LBA2        0x1F5
#define IDE_DRV         0x1F6
#define IDE_CMD         0x1F7
#define IDE_SECCOUNT1   0x1F8
#define IDE_LBA3        0x1F9
#define IDE_LBA4        0x1FA
#define IDE_LBA5        0x1FB
#define IDE_STATUS      IDE_CMD

#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_ALTSTATUS  0x0C
#define ATA_REG_DEVADDRESS 0x0D


#define IDE_CMD_READ     0x20
#define IDE_CMD_WRITE    0x30
#define IDE_CMD_IDENTIFY 0xEC

#define IDE_DRQ      (1 << 3)
#define IDE_DRDY     (1 << 6)
#define IDE_BUSY     (1 << 7)

#define IDE_CTRL_IRQ 0x3F6
#define IDE_IRQN     14

#define IDE_VENDOR_ID   PCI::VENDOR_INTEL
#define IDE_PRODUCT_ID  0x7010

#define IDE_TIMEOUT 2048

const int IDE::SECTOR_SIZE;
const int IDE::SECTOR_ARRAY;

struct workq_item
{
  using block_t = IDE::block_t;
  using buffer_t = IDE::buffer_t;

#ifdef IDE_ENABLE_READ
  using on_read_func = IDE::on_read_func;
  // read
  workq_item(uint8_t id, block_t blk, uint32_t cnt, on_read_func call)
    : drive_id(id), read(true), sector(blk), total(cnt), readcall(std::move(call))
  {
    buffer = std::make_shared<os::mem::buffer> (total * IDE::SECTOR_SIZE);
  }
#endif
#ifdef IDE_ENABLE_WRITE
  using on_write_func = IDE::on_write_func;
  // write
  workq_item(uint8_t id, block_t blk, buffer_t buf, on_write_func call)
    : drive_id(id), read(false), sector(blk), buffer(buf), writecall(std::move(call))
  {
    assert(buffer->size() % IDE::SECTOR_SIZE == 0);
    total = buffer->size() / IDE::SECTOR_SIZE;
  }
#endif
  ~workq_item() {}

  uint8_t* current() {
    return &buffer->at(position * IDE::SECTOR_SIZE);
  }
  bool done() const noexcept { return position == total; }

  uint8_t      drive_id;
  bool         read;
  block_t      sector;
  uint32_t     position = 0;
  uint32_t     total;
  buffer_t     buffer;
  union {
#ifdef IDE_ENABLE_READ
    on_read_func  readcall;
#endif
#ifdef IDE_ENABLE_WRITE
    on_write_func writecall;
#endif
  };
};
static std::deque<workq_item> work_queue;

IDE::IDE(hw::PCI_Device& pcidev, selector_t sel)
  : drive_id {(uint8_t) sel}
{
  INFO("IDE","VENDOR_ID : 0x%x, PRODUCT_ID : 0x%x",
        pcidev.vendor_id(), pcidev.product_id());
  INFO("IDE","Attaching to  PCI addr 0x%x",
        pcidev.pci_addr());

  /** PCI device checking */
  CHECKSERT(pcidev.vendor_id() == IDE_VENDOR_ID, "Vendor ID is INTEL");
  CHECKSERT(pcidev.product_id() == IDE_PRODUCT_ID, "Product ID is IDE Controller");

  /** Probe PCI resources and fetch I/O-base for device */
  pcidev.probe_resources();
  this->pci_iobase = pcidev.iobase();

  /** IRQ initialization */
  Events::get().subscribe(IDE_IRQN, {&IDE::irq_handler});
  __arch_enable_legacy_irq(IDE_IRQN);

  /** IDE device initialization */
  set_irq_mode(false);
  set_drive(0xA0 | this->drive_id);
  set_nbsectors(0U);
  set_blocknum(0U);
  set_command(IDE_CMD_IDENTIFY);

  uint8_t status = hw::inb(IDE_STATUS);
  CHECKSERT(status, "IDE status OK");

  wait_status_flags(IDE_DRDY, false);

  // read device capabilities
  std::array<uint16_t, SECTOR_ARRAY> read_array;
  for (int i = 0; i < IDE::SECTOR_ARRAY; i++) {
    read_array[i] = hw::inw(IDE_DATA);
  }
  // get ident command sets
  uint32_t command_sets = read_array[82] | (read_array[83] << 16);
  if (command_sets & (1 << 26))
  { // 48-bits MAX_LBA_EXT
    this->num_blocks = (read_array[101] << 16) | read_array[100];
  }
  else
  { // 28-bits CHS (MAX_LBA)
    this->num_blocks = (read_array[61] << 16) | read_array[60];
  }
  INFO("IDE", "%" PRIu64 " sectors (%" PRIu64 " bytes)", num_blocks, num_blocks * IDE::SECTOR_SIZE);
  INFO("IDE", "Initialization complete");
}

void IDE::read(block_t blk, size_t count, on_read_func callback)
{
  // avoid reading past the disk boundaries, or reading 0 sectors
  if (blk + count >= this->num_blocks || count == 0) {
    callback(nullptr);
    return;
  }
  IDBG("IDE: Read called on %lu + %lu\n", blk, count);

#ifdef IDE_ENABLE_READ
  work_queue.emplace_back(drive_id, blk, count, callback);
  if (work_queue.size() == 1) work_begin_next();
#else
  (void) blk;
  (void) count;
  callback(nullptr);
#endif
}

IDE::buffer_t IDE::read_sync(block_t blk, size_t cnt)
{
  if (blk >= this->num_blocks) {
    // avoid reading past the disk boundaries
    return nullptr;
  }

#ifdef IDE_ENABLE_READ
  set_irq_mode(false);
  set_drive(0xE0 | drive_id | ((blk >> 24) & 0x0F));
  set_nbsectors(cnt);
  set_blocknum(blk);
  set_command(IDE_CMD_READ);

  auto buffer = fs::construct_buffer(IDE::SECTOR_SIZE * cnt);
  wait_status_flags(IDE_DRDY, false);

  auto* data = (uint16_t*) buffer->data();
  for (size_t i = 0; i < IDE::SECTOR_ARRAY * cnt; i++)
      data[i] = hw::inw(IDE_DATA);
  return buffer;
#else
  return nullptr;
#endif
}

void IDE::write(block_t blk, buffer_t buffer, on_write_func callback)
{
#ifdef IDE_ENABLE_WRITE
  // avoid writing past the disk boundaries
  if (blk + buffer->size() / block_size() > this->num_blocks) {
    callback(true);
    return;
  }
  IDBG("IDE: Write called on %lu (%lu bytes)\n", blk, buffer->size());

  work_queue.emplace_back(drive_id, blk, buffer, callback);
  if (work_queue.size() == 1) work_begin_next();
#else
  (void) blk;
  (void) buffer;
  callback(true);
#endif
}
bool IDE::write_sync(block_t blk, buffer_t buffer)
{
#ifdef IDE_ENABLE_WRITE
  // avoid writing past the disk boundaries
  if (blk + buffer->size() / block_size() > this->num_blocks) {
    return true;
  }
  IDBG("IDE: Write called on %lu (%lu bytes)\n", blk, buffer->size());

  const uint32_t total = buffer->size() / block_size();
  set_irq_mode(false);
  set_drive(0xE0 | this->drive_id | ((blk >> 24) & 0x0F));
  set_nbsectors(total);
  set_blocknum(blk);
  set_command(IDE_CMD_WRITE);

  auto* data = (uint16_t*) buffer->data();
  for (size_t i = 0; i < total * IDE::SECTOR_ARRAY; i++)
      hw::outw(IDE_DATA, data[i]);
  return false;

#else
  (void) blk;
  (void) buffer;
  return true;
#endif
}

void IDE::spinwait_status_busy() noexcept {
  uint8_t ret;
  while (((ret = hw::inb(IDE_STATUS)) & IDE_BUSY) == IDE_BUSY) asm("pause");
}

void IDE::wait_status_flags(const int flags, const bool set) noexcept
{
  spinwait_status_busy();

  for (int i = IDE_TIMEOUT; i != 0; --i)
  {
    auto ret = hw::inb(IDE_STATUS);
    if (set) {
      if ((ret & flags) == flags)
        break;
    } else {
      if ((ret & flags) not_eq flags)
        break;
    }
  }
}

void IDE::set_drive(const uint8_t drive) noexcept {
  wait_status_flags(IDE_DRQ, true);
  hw::outb(IDE_DRV, drive);
}

void IDE::set_nbsectors(const uint8_t cnt) noexcept {
  wait_status_flags(IDE_DRQ, true);
  hw::outb(IDE_SECCNT, cnt);
}

void IDE::set_blocknum(block_t blk) noexcept
{
  wait_status_flags(IDE_DRQ, true);
  hw::outb(IDE_LBA0, blk & 0xFF);

  wait_status_flags(IDE_DRQ, true);
  hw::outb(IDE_LBA1, (blk & 0xFF00) >> 8);

  wait_status_flags(IDE_DRQ, true);
  hw::outb(IDE_LBA2, (blk & 0xFF0000) >> 16);

  wait_status_flags(IDE_DRQ, true);
  hw::outb(IDE_SECCOUNT1, 1);

  wait_status_flags(IDE_DRQ, true);
  hw::outb(IDE_LBA3, (blk & 0xFF000000) >> 24);

  wait_status_flags(IDE_DRQ, true);
  hw::outb(IDE_LBA4, 0);

  wait_status_flags(IDE_DRQ, true);
  hw::outb(IDE_LBA5, 0);
}

void IDE::set_command(const uint16_t command) noexcept {
  wait_status_flags(IDE_DRDY, false);
  hw::outb(IDE_CMD, command);
}

void IDE::set_irq_mode(const bool on) noexcept {
  wait_status_flags(IDE_DRDY, false);
  hw::outb(IDE_CTRL_IRQ, on ? 0 : 1);
}

void IDE::work_begin_next()
{
  if (work_queue.empty()) return;
  auto& item = work_queue.front();
  assert(item.position == 0);

  set_irq_mode(true);
  set_drive(0xE0 | item.drive_id | ((item.sector >> 24) & 0x0F));
#ifdef IDE_ENABLE_READ
  if (item.read) {
    set_nbsectors(item.total);
    set_blocknum(item.sector);
    set_command(IDE_CMD_READ);
  }
#endif
#ifdef IDE_ENABLE_WRITE
  if (item.read == false) {
    set_nbsectors(item.total);
    set_blocknum(item.sector);
    set_command(IDE_CMD_WRITE);
  }
#endif
}
void IDE::irq_handler()
{
  while (not work_queue.empty())
  {
    auto& item = work_queue.front();
    wait_status_flags(IDE_DRDY, false);

#ifdef IDE_ENABLE_READ
    if (item.read)
    {
      // read operation
      IDBG("IDE: Reading %u / %u\n", item.position, item.total);
      // read to current position
      auto* wptr = (uint16_t*) item.current();
      for (block_t i = 0; i < IDE::SECTOR_ARRAY; i++) {
          wptr[i] = hw::inw(IDE_DATA);
      }
      // go to next position
      item.position++;
      // if the read is done, shipit
      if (item.done())
      {
        auto buffer = std::move(item.buffer);
        auto callback = std::move(item.readcall);
        work_queue.pop_front();
        // shipit
        callback(std::move(buffer));
        // queue next job, if any
        work_begin_next();
      } // done
    } // read
#endif
#ifdef IDE_ENABLE_WRITE
    if (item.read == false)
    {
      // write operation
      IDBG("IDE: Writing %u / %u\n", item.position, item.total);
      // write from current position
      auto* wptr = (uint16_t*) item.current();
      for (block_t i = 0; i < IDE::SECTOR_ARRAY; i++) {
          hw::outw(IDE_DATA, wptr[i]);
      }
      // go to next position
      item.position++;
      // if the read is done, shipit
      if (item.done())
      {
        auto callback = std::move(item.writecall);
        work_queue.pop_front();
        // shipit
        callback(false);
        // queue next job, if any
        work_begin_next();
      } // done
    }
#endif
  } // queue
}

void IDE::deactivate() {}

#include <kernel/pci_manager.hpp>
__attribute__((constructor))
static void autoreg() {
  PCI_manager::register_blk(PCI::VENDOR_INTEL, IDE_PRODUCT_ID, &IDE::new_instance);
};
