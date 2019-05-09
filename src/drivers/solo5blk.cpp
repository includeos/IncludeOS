#define DEBUG
#define DEBUG2
#include "solo5blk.hpp"

#include <hw/pci.hpp>
#include <fs/common.hpp>
#include <cassert>
#include <stdlib.h>

#include <statman>

extern "C" {
#include <solo5/solo5.h>
}

Solo5Blk::Solo5Blk()
  : hw::Block_device()
{
  struct solo5_block_info bi;
  solo5_block_info(&bi);
  assert(bi.block_size == SECTOR_SIZE);
  INFO("Solo5Blk", "Block device with %zu sectors",
       bi.capacity / SECTOR_SIZE);
}

Solo5Blk::block_t Solo5Blk::size() const noexcept {
  struct solo5_block_info bi;
  solo5_block_info(&bi);
  return bi.capacity / SECTOR_SIZE;
}

Solo5Blk::buffer_t Solo5Blk::read_sync(block_t blk, size_t count) {
  auto buffer = fs::construct_buffer(SECTOR_SIZE * count);
  solo5_result_t res;

  auto* data = (uint8_t*) buffer->data();
  for (size_t i = 0; i < count; i++) {
    res = solo5_block_read((solo5_off_t) (blk + i) * SECTOR_SIZE,
                           data + (i * SECTOR_SIZE), SECTOR_SIZE);
    if (res != SOLO5_R_OK) {
      return nullptr;
    }
  }
  return buffer;
}

void Solo5Blk::deactivate()
{
  INFO("Solo5Net", "deactivate");
}

#include <kernel/solo5_manager.hpp>

struct Autoreg_solo5blk {
  Autoreg_solo5blk() {
    Solo5_manager::register_blk(&Solo5Blk::new_instance);
  }
} autoreg_solo5blk;
