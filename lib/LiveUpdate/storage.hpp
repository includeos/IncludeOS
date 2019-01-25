// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 IncludeOS AS, Oslo, Norway
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
 * Master thesis
 * by Alf-Andre Walla 2016-2017
 *
**/
#pragma once
#include <cstdint>
#include <array>
#include <string>
#include <vector>
#include <delegate>

enum storage_type
{
  TYPE_END     = 0,
  TYPE_MARKER  = 1,
  TYPE_INTEGER = 2,

  TYPE_STRING  = 10,
  TYPE_BUFFER  = 11,
  TYPE_VECTOR  = 12,
  TYPE_STR_VECTOR = 13,

  TYPE_TCP    = 100,
  TYPE_TCP6   = 101,
  TYPE_WEBSOCKET  = 105,
  TYPE_STREAM = 106,
};

struct segmented_entry
{
  size_t     count;
  size_t     esize;
  char       vla[0];
};

struct varseg_begin
{
  size_t count;
  char   vla[0];
};
struct varseg_entry
{
  size_t len;
  char   vla[0];
};

struct storage_entry
{
  storage_entry(int16_t type, uint16_t id, int length);

  int16_t   type = TYPE_END;
  uint16_t  id   = 0;
  int       len  = 0;
  char      vla[0];

  int length() const noexcept {
    if (type != TYPE_INTEGER)
        return len;
    else
        return sizeof(int);
  }
  int size() const noexcept {
    return sizeof(storage_entry) + length();
  }
  segmented_entry& get_segs() noexcept {
    return *(segmented_entry*) vla;
  }
  const char* data() const noexcept {
    return vla;
  }

  storage_entry* next() const noexcept;
  uint32_t       checksum() const;
};

struct partition_header
{
  uint32_t generate_checksum(const char* vla) const;

  char      name[16] = {0};
  uint32_t  crc = 0;
  uint32_t  offset = 0;
  uint32_t  length = 0;
};

struct storage_header
{
  typedef delegate<int(char*)> construct_func;
  static const uint64_t  LIVEUPD_MAGIC;

  size_t get_length() const noexcept {
    return this->length;
  }
  size_t total_bytes() const noexcept {
    return sizeof(storage_header) + get_length();
  }
  uint32_t get_entries() const noexcept {
    return this->entries;
  }

  storage_header();
  int  create_partition(std::string key);
  int  find_partition(const char*) const;
  void finish_partition(int);
  void zero_partition(int);

  void add_marker(uint16_t id);
  void add_int   (uint16_t id, int value);
  void add_string(uint16_t id, const std::string& data);
  void add_buffer(uint16_t id, const char*, int);
  storage_entry& add_struct(int16_t type, uint16_t id, int length);
  storage_entry& add_struct(int16_t type, uint16_t id, construct_func);
  void add_vector(uint16_t, const void*, size_t cnt, size_t esize);
  void add_string_vector(uint16_t id, const std::vector<std::string>& vec);
  void add_end();

  storage_entry* begin(int p);
  storage_entry* next(storage_entry*);

  template <typename... Args>
  storage_entry& create_entry(Args&&... args);

  inline storage_entry&
  var_entry(int16_t type, uint16_t id, construct_func func);

  void append_eof() noexcept {
    ((storage_entry*) &vla[length])->type = TYPE_END;
  }
  void finalize();
  bool validate() const noexcept;
  // zero out everything if all partitions consumed
  void try_zero() noexcept;

private:
  uint32_t generate_checksum() const noexcept;
  // zero out the entire header and its data, for extra security
  void zero();

  uint64_t magic;
  mutable uint32_t crc = 0;
  uint32_t entries = 0;
  uint32_t length  = 0;
  std::array<partition_header, 16> ptable;
  uint32_t partitions = 0;
  char     vla[0];
};

template <typename... Args>
inline storage_entry&
storage_header::create_entry(Args&&... args)
{
  // create entry
  auto* entry = (storage_entry*) &vla[length];
  new (entry) storage_entry(args...);
  // next storage_entry will be this much further out:
  this->length += entry->size();
  this->entries++;
  // make sure storage is properly EOF'd
  this->append_eof();
  return *entry;
}

inline storage_entry&
storage_header::var_entry(int16_t type, uint16_t id, construct_func func)
{
  // create entry
  auto* entry = (storage_entry*) &vla[length];
  new (entry) storage_entry(type, id, 0);
  // determine and set size of entry
  entry->len = func(entry->vla);
  // next storage_entry will be this much further out:
  this->length += entry->size();
  this->entries++;
  // make sure storage is properly EOF'd
  this->append_eof();
  return *entry;
}
