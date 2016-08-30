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

#include <statman>

// Stat

Stat::Stat(const stat_type type, const int index_into_span, const std::string& name)
  : type_{type}, index_into_span_{index_into_span}
{
  switch (type) {
    case UINT32:  ui32 = 0; break;
    case UINT64:  ui64 = 0; break;
    case FLOAT:   f = 0.0f; break;
    default:      throw Stats_exception{"Creating stat: Invalid stat_type"};
  }

  snprintf(name_, sizeof(name_), "%s", name.c_str());
}

void Stat::operator++() {
  switch (type_) {
    case UINT32:  ui32++; break;
    case UINT64:  ui64++; break;
    case FLOAT:   f += 1.0f; break;
    default:      throw Stats_exception{"Incrementing stat: Invalid stat_type"};
  }
}

float& Stat::get_float() {
  if(type_ not_eq FLOAT)
    throw Stats_exception{"Get stat: stat_type is not a float"};

  return f;
}

uint32_t& Stat::get_uint32() {
  if(type_ not_eq UINT32)
    throw Stats_exception{"Get stat: stat_type is not an uint32_t"};

  return ui32;
}

uint64_t& Stat::get_uint64() {
  if(type_ not_eq UINT64)
    throw Stats_exception{"Get stat: stat_type is not an uint64_t"};

  return ui64;
}

// Statman

Statman::Statman(uintptr_t start, Size_type num_bytes)
{
  if(num_bytes < 0)
    throw Stats_exception{"Creating Statman: A negative number of bytes has been given"};

  Size_type num_stats_in_span = num_bytes / sizeof(Stat);

  num_bytes_ = sizeof(Stat) * num_stats_in_span;
  stats_ = Span(reinterpret_cast<Stat*>(start), num_stats_in_span);
}

Statman::Span_iterator Statman::last_used() {
  int i = 0;

  for(auto it = stats_.begin(); it not_eq stats_.end(); ++it) {
    if(i == next_available_)
      return it;
    i++;
  }

  return stats_.end();
}

Stat& Statman::create(const Stat::stat_type type, const std::string& name) {
  int idx = next_available_;

  if(idx >= stats_.size())
    throw Stats_out_of_memory();

  return (*new (&stats_[next_available_++]) Stat{type, idx, name});
}
