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
#include <info>
#include <smp_utils>

// this is done to make sure construction only happens here
static Statman statman_instance;
Statman& Statman::get() {
  return statman_instance;
}

Stat::Stat(const Stat_type type, const std::string& name)
  : ui64(0)
{
  // stats are persisted by default
  this->m_bits = uint8_t(type) | PERSIST_BIT;

  if (name.size() > MAX_NAME_LEN)
    throw Stats_exception("Stat name cannot be longer than " + std::to_string(MAX_NAME_LEN) + " characters");

  snprintf(name_, sizeof(name_), "%s", name.c_str());
}
Stat::Stat(const Stat& other) {
  this->ui64   = other.ui64;
  this->m_bits = other.m_bits;
  __builtin_memcpy(this->name_, other.name_, sizeof(name_));
}
Stat& Stat::operator=(const Stat& other) {
  this->ui64   = other.ui64;
  this->m_bits = other.m_bits;
  __builtin_memcpy(this->name_, other.name_, sizeof(name_));
  return *this;
}

void Stat::operator++() {
  switch (this->type()) {
    case UINT32: ui32++;    break;
    case UINT64: ui64++;    break;
    case FLOAT:  f += 1.0f; break;
    default: throw Stats_exception("Invalid stat type encountered when incrementing");
  }
}

std::string Stat::to_string() const {
  switch (this->type()) {
    case UINT32: return std::to_string(ui32);
    case UINT64: return std::to_string(ui64);
    case FLOAT:  return std::to_string(f);
    default:     return "Unknown stat type";
  }
}

///////////////////////////////////////////////////////////////////////////////

Statman::Statman() {
  this->create(Stat::UINT32, "statman.unused_stats");
}

Stat& Statman::create(const Stat::Stat_type type, const std::string& name)
{
#ifdef INCLUDEOS_SMP_ENABLE
  volatile scoped_spinlock lock(this->stlock);
#endif
  if (name.empty())
    throw Stats_exception("Cannot create Stat with no name");

  const ssize_t idx = this->find_free_stat();
  if (idx < 0) {
    m_stats.emplace_back(type, name);
    return m_stats.back();
  }

  // note: we have to create this early in case it throws
  auto& stat = *new (&m_stats[idx]) Stat(type, name);
  unused_stats()--; // decrease unused stats
  return stat;
}

Stat& Statman::get(const Stat* st)
{
#ifdef INCLUDEOS_SMP_ENABLE
  volatile scoped_spinlock lock(this->stlock);
#endif
  for (auto& stat : this->m_stats) {
    if (&stat == st) {
      if (stat.unused() == false)
          return stat;
      throw Stats_exception("Accessing deleted stat");
    }
  }
  throw std::out_of_range("Not a valid stat in this statman instance");
}

Stat& Statman::get_by_name(const char* name)
{
#ifdef INCLUDEOS_SMP_ENABLE
  volatile scoped_spinlock lock(this->stlock);
#endif
  for (auto& stat : this->m_stats)
  {
    if (stat.unused() == false)
    if (strncmp(stat.name(), name, Stat::MAX_NAME_LEN) == 0)
        return stat;
  }
  throw std::out_of_range("No stat found with exact given name");
}

void Statman::free(void* addr)
{
  auto& stat = this->get((Stat*) addr);
#ifdef INCLUDEOS_SMP_ENABLE
  volatile scoped_spinlock lock(this->stlock);
#endif
  // delete entry
  new (&stat) Stat(Stat::FLOAT, "");
  unused_stats()++; // increase unused stats
}

ssize_t Statman::find_free_stat() const noexcept
{
  for (size_t i = 0; i < this->m_stats.size(); i++)
  {
    if (m_stats[i].unused()) return i;
  }
  return -1;
}

void Statman::clear()
{
  if (size() <= 1) return;
  m_stats.clear();
  this->create(Stat::UINT32, "statman.unused_stats");
}
