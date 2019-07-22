
#pragma once
#ifndef FS_PARTITION_HPP
#define FS_PARTITION_HPP

#include <cstdint>

namespace fs
{
  struct Partition {
    explicit Partition(const uint8_t  fl,  const uint8_t  Id,
                       const uint32_t LBA, const uint32_t sz) noexcept
    : flags     {fl},
      id        {Id},
      lba_begin {LBA},
      sectors   {sz}
    {}

    uint8_t  flags;
    uint8_t  id;
    uint32_t lba_begin;
    uint32_t sectors;

    // true if the partition has boot code / is bootable
    bool is_boot() const noexcept
    { return flags & 0x1; }

    // human-readable name of partition id
    std::string name() const;

    // logical block address of beginning of partition
    uint32_t lba() const
    { return lba_begin; }

  }; //< struct Partition

}

#endif
