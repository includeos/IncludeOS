// -*-C++-*-

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
