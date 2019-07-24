/**
 * Master thesis
 * by Alf-Andre Walla 2016-2017
 *
**/
#include "storage.hpp"
#include <util/crc32.hpp>
#include <cassert>
#include <cstring>
extern bool LIVEUPDATE_EXTRA_CHECKS;

inline uint32_t liu_crc32(const void* buf, size_t len)
{
  return crc32_fast(buf, len);
}

uint32_t partition_header::generate_checksum(const char* vla) const {
  return liu_crc32(&vla[this->offset], this->length);
}

int storage_header::create_partition(const std::string key)
{
  if (key.size() > sizeof(partition_header::name)-1)
      throw std::length_error("Key '" + key + "' too long");
  if (partitions >= ptable.size())
      throw std::out_of_range("Partition table is full");
  auto& part = ptable.at(partitions);
  snprintf(part.name, sizeof(part.name), "%s", key.c_str());
  part.offset = this->length;
  return partitions++;
}
int storage_header::find_partition(const char* key) const
{
  for (uint32_t p = 0; p < this->partitions; p++)
  {
    auto& part = ptable.at(p);
    if (strncmp(part.name, key, sizeof(part.name)) == 0)
    {
      // the partition must have a valid name
      assert(part.name[0] != 0);
      // the partition should be fully consistent
      if (LIVEUPDATE_EXTRA_CHECKS) {
        uint32_t chsum = part.generate_checksum(this->vla);
        if (part.crc != chsum)
        throw std::runtime_error("Invalid CRC in partition '" + std::string(key) + "'");
      }
      return p;
    }
  }
  return -1;
}
void storage_header::finish_partition(int p)
{
  // make sure partition ends properly
  this->add_end();
  // write length and crc
  auto& part = ptable.at(p);
  part.length = this->length - part.offset;
  if (LIVEUPDATE_EXTRA_CHECKS) {
    part.crc = part.generate_checksum(this->vla);
  }
  else part.crc = 0;
}
void storage_header::zero_partition(int p)
{
  auto& part = ptable.at(p);
  memset(&this->vla[part.offset], 0, part.length);
  part = {};
  if (LIVEUPDATE_EXTRA_CHECKS) {
    // NOTE: generate **NEW** checksum for header
    this->crc = generate_checksum();
  }
}
