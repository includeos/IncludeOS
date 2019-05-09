// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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

#ifndef PLUGINS_MADNESS_HPP
#define PLUGINS_MADNESS_HPP

// Cause trouble for services. Useful for robustness testing.

namespace madness {
  using namespace std::chrono;
  constexpr auto alloc_freq = 1s;
  constexpr auto dealloc_delay = 5s;
  constexpr auto alloc_restart_delay = 60s;
  constexpr size_t alloc_min  = 0x1000;

  /* Periodically allocate all possible memory, before releasing */
  void init_heap_steal();
  void init_status_printing();
  void init();

} // namespace madness
#endif
