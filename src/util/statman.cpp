// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

#include <statman>
#include <iterator>
#include <info>
#include <smp_utils>

__attribute__((weak))
Statman& Statman::get() {
  static Statman statman;
  return statman;
}

///////////////////////////////////////////////////////////////////////////////
Stat::Stat(const Stat_type type, const std::string& name)
  : ui64(0), type_{type}
{
  if(name.size() > MAX_NAME_LEN)
    throw Stats_exception{"Creating stat: Name cannot be longer than " + std::to_string(MAX_NAME_LEN) + " characters"};

  snprintf(name_, sizeof(name_), "%s", name.c_str());
}

void Stat::operator++() {
  switch (type_) {
    case UINT32: ui32++;    break;
    case UINT64: ui64++;    break;
    case FLOAT:  f += 1.0f; break;
    default:     throw Stats_exception{"Incrementing stat: Invalid Stat_type"};
  }
}

float& Stat::get_float() {
  if (type_ not_eq FLOAT)
    throw Stats_exception{"Get stat: Stat_type is not a float"};

  return f;
}

uint32_t& Stat::get_uint32() {
  if (type_ not_eq UINT32)
    throw Stats_exception{"Get stat: Stat_type is not an uint32_t"};

  return ui32;
}

uint64_t& Stat::get_uint64() {
  if (type_ not_eq UINT64)
    throw Stats_exception{"Get stat: Stat_type is not an uint64_t"};

  return ui64;
}

///////////////////////////////////////////////////////////////////////////////


void Statman::init(const uintptr_t start, const Size_type num_bytes)
{
  if (num_bytes < 0)
    throw Stats_exception("Can't initialize statman with negative size");

  const int N = num_bytes / sizeof(Stat);
  this->stats_     = reinterpret_cast<Stat*>(start);
  this->end_stats_ = this->stats_ + N;

  // create bitmap
  int chunks = N / (sizeof(MemBitmap::word) * 8) + 1;
  delete[] bdata;
  bdata = new MemBitmap::word[chunks]();
  bitmap = MemBitmap(bdata, chunks);

  INFO("Statman", "Initialized with %u stats capacity", (uint32_t) capacity());
}
Statman::~Statman()
{
  delete[] bdata;
}

Stat& Statman::create(const Stat::Stat_type type, const std::string& name) {
#ifndef INCLUDEOS_SINGLE_THREADED
  volatile scoped_spinlock lock(this->stlock);
#endif
  if (name.empty())
    throw Stats_exception("Cannot create Stat with no name");

  const int idx = bitmap.first_free();
  if (idx == -1 || idx >= capacity())
    throw Stats_out_of_memory{};

  // note: we have to create this early in case it throws
  auto& stat = *new (&stats_[idx]) Stat(type, name);
  bitmap.set(idx);
  return stat;
}

Stat& Statman::get(const void* addr)
{
  auto* st = (Stat*) addr;
  auto  mod  = ((uintptr_t) st - (uintptr_t) stats_) % sizeof(Stat);

  if (st >= cbegin() && st < cend())
  if (mod == 0) // verify stat boundary
  {
    if (st->unused() == false)
      return *st;
    else
      throw Stats_exception{"Accessing deleted stat"};
  }

  throw std::out_of_range("Address out of range");
}

Stat& Statman::get_by_name(const char* name)
{
  for (auto* st = begin(); st < end(); st++)
  {
    if (st->unused() == false)
    if (strncmp(st->name(), name, Stat::MAX_NAME_LEN) == 0)
        return *st;
  }
  throw std::out_of_range("No stat found with exact given name");
}

void Statman::free(void* addr)
{
  auto& stat = this->get(addr);
  auto  idx  = &stat - cbegin();

  // delete entry
  new (&stat) Stat(Stat::FLOAT, "");
  bitmap.reset(idx);
}
