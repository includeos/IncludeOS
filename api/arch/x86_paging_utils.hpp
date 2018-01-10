// -*-C++-*-
// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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

#ifndef X86_PAGING_UTILS
#define X86_PAGING_UTILS

#include <iostream>
#include <map>

namespace x86 {
namespace paging {

using Pflag = Flags;


inline std::ostream& operator<<(std::ostream& out, const Pflag& flag) {
static const std::map<Pflag, const char*> flag_names {
  {Pflag::present,    "Present"},
  {Pflag::writable,   "Writable"},
  {Pflag::user,       "User"},
  {Pflag::write_thr,  "Write-through cache"},
  {Pflag::cache_dis,  "Cache disable"},
  {Pflag::accessed,   "Accessed"},
  {Pflag::dirty,      "Dirty"},
  {Pflag::huge,       "Huge_page"},
  {Pflag::global,     "Global"},
  {Pflag::pdir,       "Page dir"},
  {Pflag::no_exec,    "NX"}
};

  out << " [ ";
  for (auto& fl : flag_names) {
    if (has_flag(flag , fl.first))
      out << fl.second << " | ";

    if (flag == Pflag::none) {
      out << "No flags ";
      break;
    }
  }

  if ((flag & Pflag::all) == Pflag::all)
    out << " | (All flags) ";

  return out << std::hex
    << "(0x" << static_cast<uintptr_t>(flag) << ") ]";
}


}
}

#endif
