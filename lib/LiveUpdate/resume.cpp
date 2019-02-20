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
#include "liveupdate.hpp"

#include <os.hpp>
#include <kernel.hpp>
#include "storage.hpp"
#include "serialize_tcp.hpp"
#include <cstdio>

//#define LPRINT(x, ...) printf(x, ##__VA_ARGS__);
#define LPRINT(x, ...) /** x **/

namespace liu
{
static bool resume_begin(storage_header&, std::string, LiveUpdate::resume_func);

bool LiveUpdate::is_resumable()
{
  return is_resumable(kernel::liveupdate_storage_area());
}
bool LiveUpdate::is_resumable(const void* location)
{
  return ((const storage_header*) location)->validate();
}
bool LiveUpdate::os_is_liveupdated() noexcept
{
  return kernel::state().is_live_updated;
}

static bool resume_helper(void* location, std::string key, LiveUpdate::resume_func func)
{
  // check if an update has occurred
  if (!LiveUpdate::is_resumable(location))
      return false;

  LPRINT("* Restoring data...\n");
  // restore connections etc.
  return resume_begin(*(storage_header*) location, key.c_str(), func);
}
bool LiveUpdate::resume(std::string key, resume_func func)
{
  void* location = kernel::liveupdate_storage_area();
  /// memory sanity check
  if (kernel::heap_end() >= (uintptr_t) location) {
    fprintf(stderr,
        "WARNING: LiveUpdate storage area inside heap (margin: %ld)\n",
		     (long int) (kernel::heap_end() - (uintptr_t) location));
    throw std::runtime_error("LiveUpdate::resume(): Storage area inside heap");
  }
  return resume_helper(location, std::move(key), func);
}
bool LiveUpdate::partition_exists(const std::string& key, const void* area) noexcept
{
  if (area == nullptr) area = kernel::liveupdate_storage_area();

  if (!LiveUpdate::is_resumable(area))
    return false;

  auto& storage = *(const storage_header*) area;
  return (storage.find_partition(key.c_str()) != -1);
}
void LiveUpdate::resume_from_heap(void* location, std::string key, LiveUpdate::resume_func func)
{
  resume_helper(location, std::move(key), func);
}

bool resume_begin(storage_header& storage, std::string key, LiveUpdate::resume_func func)
{
  if (key.empty())
      throw std::length_error("LiveUpdate partition key cannot be an empty string");

  int p = storage.find_partition(key.c_str());
  if (p == -1) return false;
  LPRINT("* Resuming from partition %d at %p from %p\n",
        p, storage.begin(p), &storage);

  // resume wrapper
  Restore wrapper(storage.begin(p));
  // use registered functions when we can, otherwise, use normal
  func(wrapper);

  // wake all the slumbering IP stacks
  serialized_tcp::wakeup_ip_networks();
  // zero out the partition for security reasons
  storage.zero_partition(p);
  // if there are no more partitions, clear everything
  storage.try_zero();
  return true;
}

/// struct Restore

bool Restore::is_end() const noexcept
{
  return get_type() == TYPE_END;
}
bool Restore::is_marker() const noexcept
{
  return get_type() == TYPE_MARKER;
}
bool Restore::is_int() const noexcept
{
  return get_type() == TYPE_INTEGER;
}
bool Restore::is_string() const noexcept
{
  return get_type() == TYPE_STRING;
}
bool Restore::is_buffer() const noexcept
{
  return get_type() == TYPE_BUFFER;
}
bool Restore::is_vector() const noexcept
{
  return get_type() == TYPE_VECTOR;
}
bool Restore::is_string_vector() const noexcept
{
  return get_type() == TYPE_STR_VECTOR;
}
bool Restore::is_stream() const noexcept
{
  return get_type() == TYPE_STREAM;
}

int Restore::as_int() const
{
  // this type uses the length field directly as value to save space
  if (ent->type == TYPE_INTEGER)
      return ent->len;
  throw std::runtime_error("LiveUpdate: Restore::as_int() encountered incorrect type " + std::to_string(ent->type));
}
std::string Restore::as_string() const
{
  if (ent->type == TYPE_STRING)
      return std::string(ent->data(), ent->len);
  throw std::runtime_error("LiveUpdate: Restore::as_string() encountered incorrect type " + std::to_string(ent->type));
}
buffer_t  Restore::as_buffer() const
{
  if (ent->type == TYPE_BUFFER) {
      buffer_t buffer;
      buffer.assign(ent->data(), ent->data() + ent->len);
      return buffer;
  }
  throw std::runtime_error("LiveUpdate: Restore::as_buffer() encountered incorrect type " + std::to_string(ent->type));
}

int16_t     Restore::get_type() const noexcept
{
  return ent->type;
}
uint16_t    Restore::get_id() const noexcept
{
  return ent->id;
}
int32_t     Restore::length() const noexcept
{
  return ent->len;
}
const void* Restore::data() const noexcept
{
  return ent->data();
}
const void* Restore::get_segment(size_t size, size_t& count) const
{
  if (ent->type != TYPE_VECTOR)
      throw std::runtime_error("LiveUpdate: Restore::as_vector() encountered incorrect type " + std::to_string(ent->type));

  auto& segs = ent->get_segs();
  if (size != segs.esize)
      throw std::runtime_error("LiveUpdate: Restore::as_vector() encountered incorrect type size " + std::to_string(size) + " vs " + std::to_string(segs.esize));

  count = segs.count;
  return (const void*) segs.vla;
}
std::vector<std::string> Restore::rebuild_string_vector() const
{
  if (ent->type != TYPE_STR_VECTOR)
      throw std::runtime_error("LiveUpdate: Restore::as_vector<std::string>() encountered incorrect type " + std::to_string(ent->type));
  std::vector<std::string> retv;
  // reserve just enough room
  auto* begin = (varseg_begin*) ent->vla;
  retv.reserve(begin->count);

  auto* el = (varseg_entry*) begin->vla;
  for (size_t i = 0; i < begin->count; i++)
  {
    //printf("Restoring string len=%u: %.*s\n", el->len, el->len, el->vla);
    retv.emplace_back(el->vla, el->len);
    // next
    el = (varseg_entry*) &el->vla[el->len];
  }
  return retv;
}

///
void     Restore::go_next()
{
  if (is_end())
      throw std::out_of_range("Restore::go_next(): Already reached end of partition");
  // increase the counter, so the resume loop skips entries properly
  ent = ent->next();
}
uint16_t Restore::next_id() const noexcept
{
  return ent->next()->id;
}

uint16_t Restore::pop_marker()
{
  uint16_t result = 0;
  while (is_marker() == false
      && is_end() == false) go_next();
  if (is_marker()) {
    result = get_id();
    go_next();
  }
  return result;
}
void Restore::pop_marker(uint16_t id)
{
  while (is_marker() == false
      && is_end() == false) go_next();
  if (is_marker()) {
    if (get_id() != id)
        throw std::out_of_range("Restore::pop_marker(): Ran past marker with another id: " + std::to_string(get_id()));
    go_next();
  }
}

} // liu
