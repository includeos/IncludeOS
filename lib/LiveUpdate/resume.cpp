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

#include <kernel/os.hpp>
#include "storage.hpp"
#include "serialize_tcp.hpp"
#include <cstdio>
#include <map>

//#define LPRINT(x, ...) printf(x, ##__VA_ARGS__);
#define LPRINT(x, ...) /** x **/

// heap area
extern char* heap_end;

namespace liu
{
static std::map<uint16_t, LiveUpdate::resume_func> resume_funcs;

bool LiveUpdate::is_resumable()
{
  return is_resumable(OS::liveupdate_storage_area());
}
bool LiveUpdate::is_resumable(void* location)
{
  return ((storage_header*) location)->validate();
}

static bool resume_helper(void* location, LiveUpdate::resume_func func)
{
  // check if an update has occurred
  if (!LiveUpdate::is_resumable(location)) return false;

  LPRINT("* Restoring data...\n");
  // restore connections etc.
  extern bool resume_begin(storage_header&, LiveUpdate::resume_func);
  return resume_begin(*(storage_header*) location, func);
}
bool LiveUpdate::resume(resume_func func)
{
  void* location = OS::liveupdate_storage_area();
  /// memory sanity check
  if (heap_end >= (char*) location) {
    fprintf(stderr,
        "WARNING: LiveUpdate storage area inside heap (margin: %ld)\n",
		     (long int) (heap_end - (char*) location));
    return false;
  }
  return resume_helper(location, func);
}
bool LiveUpdate::resume_from_heap(void* location, LiveUpdate::resume_func func)
{
  return resume_helper(location, func);
}

bool resume_begin(storage_header& storage, LiveUpdate::resume_func func)
{
  /// restore each entry one by one, calling registered handlers
  auto num_ents = storage.get_entries();
  if (num_ents > 1) {
    LPRINT("* Resuming %d stored entries\n", num_ents-1);
  } else {
    LPRINT("* No stored entries to resume\n");
  }

  for (auto* ptr = storage.begin(); ptr->type != TYPE_END;)
  {
    auto* oldptr = ptr;
    // resume wrapper
    Restore wrapper {ptr};
    // use registered functions when we can, otherwise, use normal
    auto it = resume_funcs.find(ptr->id);
    if (it != resume_funcs.end())
    {
      it->second(wrapper);
    } else {
      func(wrapper);
    }
    // if we are already at the end due calls to go_next, break early
    if (ptr->type == TYPE_END) break;
    // call next manually only when no one called go_next
    if (oldptr == ptr) ptr = storage.next(ptr);
  }
  /// wake all the slumbering IP stacks
  serialized_tcp::wakeup_ip_networks();
  /// zero out all the state for security reasons
  storage.zero();

  return true;
}

void LiveUpdate::on_resume(uint16_t id, resume_func func)
{
  resume_funcs[id] = func;
}

/// struct Restore

bool Restore::is_end() const noexcept
{
  return get_type() == TYPE_END;
}
bool Restore::is_int() const noexcept
{
  return get_type() == TYPE_INTEGER;
}
bool Restore::is_marker() const noexcept
{
  return get_type() == TYPE_MARKER;
}

int Restore::as_int() const
{
  // this type uses the length field directly as value to save space
  if (ent->type == TYPE_INTEGER)
      return ent->len;
  throw std::runtime_error("Incorrect type: " + std::to_string(ent->type));
}
std::string Restore::as_string() const
{
  if (ent->type == TYPE_STRING)
      return std::string(ent->data(), ent->len);
  throw std::runtime_error("Incorrect type: " + std::to_string(ent->type));
}
buffer_t  Restore::as_buffer() const
{
  if (ent->type == TYPE_BUFFER) {
      buffer_t buffer;
      buffer.assign(ent->data(), ent->data() + ent->len);
      return buffer;
  }
  throw std::runtime_error("Incorrect type: " + std::to_string(ent->type));
}
Restore::Connection_ptr Restore::as_tcp_connection(net::TCP& tcp) const
{
  return deserialize_connection(ent->vla, tcp);
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
      throw std::runtime_error("Incorrect type: " + std::to_string(ent->type));

  auto& segs = ent->get_segs();
  if (size != segs.esize)
      throw std::runtime_error("Incorrect type size: " + std::to_string(size) + " vs " + std::to_string(segs.esize));

  count = segs.count;
  return (const void*) segs.vla;
}
std::vector<std::string> Restore::rebuild_string_vector() const
{
  if (ent->type != TYPE_STR_VECTOR)
      throw std::runtime_error("Incorrect type: " + std::to_string(ent->type));
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
      throw std::runtime_error("Already reached end of storage");
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
        throw std::runtime_error("Ran past marker with another id: " + std::to_string(get_id()));
    go_next();
  }
}

void Restore::cancel()
{
  while (is_end() == false) go_next();
}

// copy operator
Restore::Restore(const Restore& other)
  : ent(other.ent)  {}

}
