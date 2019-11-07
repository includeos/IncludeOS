
#include <cassert>
#include <common>
#include <delegate>
#include <vector>

#include <kernel/solo5_manager.hpp>
#include <stdexcept>
#include <hal/machine.hpp>

static std::vector<delegate<Solo5_manager::Nic_ptr()>> nics;
static std::vector<delegate<Solo5_manager::Blk_ptr()>> blks;

void Solo5_manager::register_net(delegate<Nic_ptr()> func)
{
  nics.push_back(func);
}
void Solo5_manager::register_blk(delegate<Blk_ptr()> func)
{
  blks.push_back(func);
}

void Solo5_manager::init() {
  INFO("Solo5", "Looking for solo5 devices");

  for (auto nic : nics)
    os::machine().add<hw::Nic> (nic());
  for (auto blk : blks)
    os::machine().add<hw::Block_device> (blk());
}
