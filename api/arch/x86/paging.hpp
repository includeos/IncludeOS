// -*-C++-*-
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

#ifndef X86_PAGING_HPP
#define X86_PAGING_HPP

#include <common>
#include <array>
#include <delegate>
#include <util/bitops.hpp>
#include <util/units.hpp>
#include <kernel/memory.hpp>
#include <cstdlib>

//#define DEBUG_X86_PAGING

#ifdef DEBUG_X86_PAGING
#define PG_PRINT(X, ...) printf("<%s @ 0x%lx>" X, pg_name(page_size), start_addr(), ##__VA_ARGS__)
#else
#define PG_PRINT(X, ...) /* X */
#endif

inline const char* pg_name(uintptr_t sz){
  using namespace util::literals;
  switch (sz) {
  case 4_KiB:
    return "PML1(4k)";
  case 2_MiB:
    return "PML2(2Mb)";
  case 1_GiB:
    return "PML3(Gb)";
  case 512_GiB:
    return "PML4";
  default:
    return "N/A";
  }
};

namespace x86 {
namespace paging {

using namespace util::literals;

/**
 * x86 page entry flags.
 * Intel manual Vol.3A 4-28
 **/
enum class Flags : uintptr_t {
  none       = 0x0,
  present    = 0x1,
  writable   = 0x2,
  user       = 0x4,
  write_thr  = 0x8,
  cache_dis  = 0x10,
  accessed   = 0x20,
  dirty      = 0x40,
  huge       = 0x80,
  global     = 0x100,
  pdir       = 0x10000000000000,  // Using ignored bit 52 as pdir marker
  ign_53     = 0x20000000000000,
  ign_54     = 0x40000000000000,
  ign_55     = 0x80000000000000,
  ign_56     = 0x100000000000000,
  ign_57     = 0x200000000000000,
  ign_58     = 0x400000000000000,
  pky_59     = 0x800000000000000,
  pky_60     = 0x1000000000000000,
  pky_61     = 0x2000000000000000,
  pky_62     = 0x4000000000000000,
  no_exec    = 0x8000000000000000,

  // Flag groups
  all = (0xfff | pdir | no_exec),
  permissive = (present | writable)
};

using Map = os::mem::Mapping<Flags>;

/** x86_64 specific types for page directories **/
struct x86_64 {
  using Flags = x86::paging::Flags;
  static constexpr int min_pagesize     = 4_KiB;
  static constexpr int table_size       = 512;
  static constexpr int max_pagesize     = 1_GiB;
  static constexpr uintptr_t max_memory = 512_GiB * 512;
};

static constexpr uintptr_t any_addr = std::numeric_limits<uintptr_t>::max();

}
}

/** Enable bitmask operators for paging flags **/
namespace util {
inline namespace bitops {
template<>
struct enable_bitmask_ops<x86::paging::Flags> {
  using type = std::underlying_type<x86::paging::Flags>::type;
  static constexpr bool enable = true;
};

template<>
struct enable_bitmask_ops<decltype(4_KiB)> {
  using type = decltype(4_KiB);
  static constexpr bool enable = true;
};
}
}

namespace x86 {
namespace paging {

using namespace util::literals;
using namespace util::bitops;

/** Conversion from x86 paging flags to mem::Accessflags **/
os::mem::Access to_memflags(Flags f);

/** Conversion from mem::Access flags to x86 paging flags **/
Flags to_x86(os::mem::Access prot);

/** Summary of currently mapped page- and page directories **/
struct Summary {
  int pages_4k = 0;
  int pages_2m = 0;
  int pages_1g = 0;
  int dirs_2m = 0;
  int dirs_1g = 0;
  int dirs_512g = 0;

  void add_dir(size_t ps){
    switch (ps) {
    case 2_MiB:
      dirs_2m++;
      break;
    case 1_GiB:
      dirs_1g++;
      break;
    case 512_GiB:
      dirs_512g++;
    }
  }

  void add_page(size_t ps){
    switch (ps) {
    case 4_KiB:
        pages_4k++;
        break;
    case 2_MiB:
      pages_2m++;
      break;
    case 1_GiB:
      pages_1g++;
      break;
    }
  }

  Summary operator+=(Summary rhs){
    pages_4k   += rhs.pages_4k;
    pages_2m   += rhs.pages_2m;
    pages_1g   += rhs.pages_1g;
    dirs_2m    += rhs.dirs_2m;
    dirs_1g    += rhs.dirs_1g;
    dirs_512g  += rhs.dirs_512g;
    return *this;
  }
};

inline std::ostream& operator<<(std::ostream& out, const Summary& sum){
  out << "Pml4 (512 * 512 GiB) \n"
      << "\t* Mapped 512 GiB page dirs : " << sum.dirs_512g << "\n"
      << "Pml3 (512 GiB) \n"
      << "\t* Mapped 1G page dirs      : " << sum.dirs_1g << "\n"
      << "\t* Mapped 1G pages          : " << sum.pages_1g << "\n"
      << "Pml2 (1 GiB) \n"
      << "\t* Mapped 2 MiB page dirs   : " << sum.dirs_2m << "\n"
      << "\t* Mapped 2 MiB pages       : " << sum.pages_2m << "\n"
      << "Pml1 (2 MiB) \n"
      << "\t* Mapped 4 KiB pages       : " << sum.pages_4k << "\n";
  return out;
}

/**
 * Multi-level page directories / page tables
 * (x86) Backend for os::mem::map / os::mem::protect
 **/
template <uintptr_t Psz, typename Sub = void,
          typename Arc = x86_64,
          typename Arc::Flags Afl = Arc::Flags::all>
class Page_table {
public:
  using Arch    = Arc;
  using Pflag   = typename Arch::Flags;
  using Subdir  = Sub;
  static constexpr uintptr_t page_size     = Psz;
  static constexpr uintptr_t min_pagesize  = Arch::min_pagesize;
  static constexpr uintptr_t range_size    = page_size * Arch::table_size;
  static constexpr Pflag     allowed_flags = Afl;

  constexpr size_t size() {
    return tbl_.size();
  }

  Page_table() = default;
  Page_table(uintptr_t vstart)
    : linear_addr_start_{vstart}
  {
    //static_assert(std::is_pod<decltype(*this)>::value, "Page table must be POD");
    static_assert(std::is_pod<decltype(tbl_)>::value,
                  "Page table must have standard layout");
    static_assert(offsetof(Page_table, tbl_) == 0);
    static_assert(alignof(Page_table) == Arch::min_pagesize);
    Expects((((uintptr_t)this) & ((uintptr_t)(4_KiB - 1))) == 0);
  }
  Page_table(uintptr_t lin, Pflag flags)
    : Page_table(lin)
  {
    // Default to identity-mapping
    id_map(flags);
  }

  Page_table(uintptr_t lin, uintptr_t phys, Pflag flags)
    : Page_table(lin)
  {
    map_all(phys, flags);
  }

  Page_table(const Page_table&)  = delete;
  Page_table(const Page_table&&) = delete;
  Page_table& operator=(const Page_table&) = delete;
  Page_table& operator=(Page_table&&) = delete;

  ~Page_table() {
    if constexpr (not std::is_void_v<Subdir>) {
      for (auto& ent : tbl_) {
        if (is_page_dir(ent)) {
          Subdir* ptr = page_dir(&ent);
          delete ptr;
        }
      }
    }
  }

  static constexpr bool is_page_aligned(uintptr_t addr) noexcept
  { return (addr & (page_size - 1)) == 0; }

  static bool is_range_aligned(uintptr_t addr) noexcept
  { return (addr & (range_size - 1)) == 0; }

  static bool is_page(uintptr_t entry) noexcept
  { return ((entry & Pflag::huge & allowed_flags) or page_size == min_pagesize)
      and !(entry & Pflag::pdir) and is_page_aligned(addr_of(entry)); }

  static uintptr_t addr_of(uintptr_t entry) noexcept
  { return entry & ~Pflag::all; }

  static void* to_addr(uintptr_t entry) noexcept
  { return reinterpret_cast<void*>(addr_of(entry)); }

  static bool is_page_dir(uintptr_t entry) noexcept
  { return (entry & Pflag::pdir) and !is_page(entry) and addr_of(entry) != 0; }

  static Pflag flags_of(uintptr_t entry) noexcept
  { return static_cast<Pflag>(entry & allowed_flags); }

  bool has_flag(uintptr_t entry, Pflag fl) noexcept
  {
    auto* ent = entry_r(entry);
    if (ent == nullptr) return false;
    return util::has_flag(flags_of(*ent), fl);
  }

  uintptr_t start_addr() const
  { return linear_addr_start_; }

  bool within_range(uintptr_t addr) {
    return addr >= start_addr()
      and addr < (start_addr() + page_size * tbl_.size());
  }

  int indexof(uintptr_t addr) {
    if (not within_range(addr))
        return -1;

    int64_t i = (addr - this->start_addr()) / page_size;
    Ensures(i >= 0 and static_cast<size_t>(i) < tbl_.size());
    return i;
  }


  Summary summary(bool print = false, int print_lvl = 0)
  {
    Summary sum;
    const char* pad = "|  |  |  |  |  |  |  |  |  |  |  |  |  |";
    for (auto& ent : tbl_) {
      if (is_page(ent) and ((flags_of(ent) & Pflag::present) != Pflag::none)){
        sum.add_page(this->page_size);
        continue;
      }

      if (is_page_dir(ent)){
        sum.add_dir(this->page_size);
        auto* sub = page_dir(&ent);
        Expects(sub != nullptr);
        if (print) {
          printf("%.*s-+<%s> 0x%zx\n", print_lvl * 2, pad,
                 util::Byte_r(page_size).to_string().c_str(), (void*)sub->start_addr());
        }
        sum += sub->summary(print, print_lvl + 1);
      }
    }
    if (print) {
      if (print_lvl)
        printf("%.*so\n", (print_lvl * 2) - 1 , pad);
      else
        printf("o\n");
    }
    return sum;
  }

  /** Get the page entry enclosing addr **/
  uintptr_t* entry(uintptr_t addr){
    auto index = indexof(addr);
    if (index < 0) return nullptr;
    return &tbl_.at(index);
  }


  /** Recursively get the innermost page entry representing addr **/
  uintptr_t* entry_r(uintptr_t addr){
    auto* entry_ = entry(addr);
    if (entry_ == nullptr) return nullptr;

    if (is_page_dir(*entry_))
    {
      return page_dir(entry_)->entry_r(addr);
    }
    return entry_;
  }


  /**
   * Allocate a page directory for memory range starting at addr.
   * applying flags + present
   **/
  Subdir* create_page_dir(uintptr_t lin, uintptr_t phys, Pflag flags = Pflag::none)
  {
    Expects(is_page_aligned(lin));
    Expects(is_page_aligned(phys));
    auto* entry_ = entry(lin);
    Expects(entry_ != nullptr);

    // Allocate entry
    Subdir* sub;
    sub = new Subdir(lin, phys, flags);

    Expects(sub == sub->data());
    Expects(util::bits::is_aligned<4_KiB>(sub));
    *entry_ = reinterpret_cast<uintptr_t>(sub)
      | (flags & ~Pflag::huge) | Pflag::present | Pflag::pdir;

    return sub;
  }

  ssize_t bytes_allocated()
  {
    ssize_t bytes = sizeof(Page_table);
    if constexpr (not std::is_void_v<Subdir>)
    {
      for (auto& ent : tbl_) {
        if (is_page_dir(ent)) {
          Subdir* ptr = page_dir(&ent);
          bytes += ptr->bytes_allocated();
        }
      }
    }
    return bytes;
  }

  bool is_empty()
  {
    for (auto& ent : tbl_) {
      if (addr_of(ent) != 0 and this->has_flag(ent, Pflag::present))
        return false;
    }
    return true;
  }

  ssize_t purge_unused()
  {
    ssize_t bytes = sizeof(Page_table);
    if constexpr (not std::is_void_v<Subdir>)
    {
      for (auto& ent : tbl_) {
        if (is_page_dir(ent)) {
          Subdir* dir = page_dir(&ent);
          if (dir->is_empty()) {
            PG_PRINT("Purging %p \n", (void*)addr_of(ent));
            delete dir;
            ent = 0;
            bytes += sizeof(dir);
          } else {
            bytes += dir->purge_unused();
          }
        }
      }
    }
    return bytes;
  }


  /**
   * Get page dir pointed to by entry. Entry must be created by a previous
   * call to create_page_dir
   **/
  Subdir* page_dir(uintptr_t* entry)
  {
    Expects(entry != nullptr);
    Expects(entry >= tbl_.begin() and entry < tbl_.end());
    Expects(is_page_dir(*entry));

    if (*entry == 0)
      return nullptr;

    return reinterpret_cast<Subdir*>(to_addr(*entry));
  }


  /** Recursively get the page size of the current page enclosing addr **/
  uintptr_t active_page_size(uintptr_t addr)
  {
    auto index = indexof(addr);
    if (index < 0) return 0;

    auto* entry_ = &tbl_.at(index);

    if (! is_page_dir(*entry_)) {
      return page_size;
    }

    return page_dir(entry_)->active_page_size(addr);
  }


  /** Recursively get the page size of the current page enclosing addr **/
  uintptr_t active_page_size(void* ptr)
  {
    return active_page_size(reinterpret_cast<uintptr_t>(ptr));
  }


  /**
   * Map all entries in this table to the continous range starting at phys,
   * incrementing by page size
   **/
  void map_all(uintptr_t phys, Pflag flags)
  {
    if (!is_range_aligned(phys)) {
      printf("<map_all> Physical addr 0x%lx is not range aligned \n", phys);
    }
    Expects(is_range_aligned(phys));

    for(auto& page : this->tbl_) {
      page = phys | flags;;
      phys += page_size;
    }
  }

  void id_map(Pflag flags){
    map_all(start_addr(), flags);
  }

  /**
   * Set flags on a page dir, needed to allow certain flags to have effect
   * on a page lower in the hierarchy. (E.g. allowing write on a page requires
   * write to be allowed on the enclosing page dir as well.)
   **/
  void permit_flags(uintptr_t* entry, Pflag flags){
    Expects(entry >= tbl_.begin() && entry < tbl_.end());

    auto curfl = flags_of(*entry);
    *entry |= flags & allowed_flags & Pflag::permissive;

    if (util::has_flag(curfl, Pflag::no_exec)
        && ! util::has_flag(flags, Pflag::no_exec)) {
      *entry &= ~Pflag::no_exec;
      Ensures((*entry & Pflag::no_exec) == 0);
    }
  }


  /** Set flags on a given entry pointer **/
  Pflag set_flags(uintptr_t* entry, Pflag flags)
  {
    Expects(entry >= tbl_.begin() && entry < tbl_.end());
    auto new_entry = addr_of(*entry) | (flags & allowed_flags);
    if (!is_page_aligned(addr_of(new_entry))) {
      printf("%p is not page aligned \n", (void*)addr_of(new_entry));
    }
    Expects(is_page_aligned(addr_of(new_entry)));

    // Can only make pages and page dirs present. E.g. no unmapped PML4 entry.
    if (util::has_flag(flags, Pflag::present)
        and !is_page(new_entry) and !is_page_dir(new_entry))
    {
      PG_PRINT("<set_flags> Can't set flags on non-aligned entry ");
      return Pflag::none;
    }

    *entry = new_entry;
    Ensures(flags_of(*entry) == (flags & allowed_flags));
    return flags_of(*entry);
  }


  /** Set flags on a given entry **/
  Pflag set_flags(uintptr_t addr, Pflag flags)
  {
    auto* ent = entry(addr);
    return set_flags(ent, flags);
  }


  /** Set page-specific flags (e.g. non page-dir) on a given entry **/
  Pflag set_page_flags(uintptr_t* entry, Pflag flags)
  {
    flags |= Pflag::huge;
    return set_flags(entry, flags);
  }


  /**
   * Recursively set flags for an entry
   * Permissive flags (e.g. write / present) are appended to parent entres as needed
   **/
  Pflag set_flags_r(uintptr_t addr, Pflag flags)
  {
    auto* ent = entry(addr);

    Expects(ent != nullptr);

    // Local mapping
    if (! is_page_dir(*ent)) {

      if (! is_page(*ent))
        return Pflag::none;

      set_flags(addr, flags);
      Ensures(flags_of(*ent) == flags);
      return flags_of(*ent);
    }

    // Subdir mapping
    permit_flags(ent, flags);
    return page_dir(ent)->set_flags_r(addr, flags);
  }


  /**
   * Map linear addr to physical range starting at phys. Up to size bytes,
   * rounded up to the nearest page_size, can be mapped. Used by map_r.
   * @returns linear map describing what was mapped.
   **/
  Map map(Map req)
  {
    using namespace util;
    PG_PRINT("<map> %s\n", req.to_string().c_str());
    auto offs = indexof(req.lin);
    if (offs < 0) {
      PG_PRINT("<map> Got invalid offset 0x%i for req. %s \n", offs, req.to_string().c_str());
      return Map();
    }

    Map res {req.lin, req.phys, req.flags, 0, page_size};

    for (auto it = tbl_.begin() + offs;
         it != tbl_.end() and res.size < req.size;
         it++, res.size += page_size)
    {
      if (req.phys != any_addr) {
        *it = req.phys;
        req.phys += page_size;
      } else {
        PG_PRINT("Req. any, have: 0x%lx \n", addr_of(*it));
      }
      set_page_flags(&(*it), req.flags);
    }

    Ensures(res);
    Ensures(bits::is_aligned<page_size>(res.size));

    return res;
  }

  /** Map a single entry without creating / descending into sub tables **/
  Map map_entry(uintptr_t* ent, Map req)
  {
    PG_PRINT("<map_entry> %s\n", req.to_string().c_str());
    Expects(ent != nullptr);
    Expects(ent >= tbl_.begin() && ent < tbl_.end());
    Expects(req);
    Expects(not is_page_dir(*ent));

    if (req.phys != any_addr) {
      *ent = req.phys;
    } else {
      PG_PRINT("Req. any, have: 0x%lx \n", addr_of(*ent));
    }

    req.flags = set_page_flags(ent, req.flags);
    req.size = page_size;
    req.page_sizes = page_size;

    if (addr_of(*ent) != req.phys) {
      printf("Couldn't set address: req. expected 0x%lx, got 0x%lx\n", req.phys, addr_of(*ent));
    }
    Ensures(addr_of(*ent) == req.phys);
    return req;
  }


  Map map_entry_r(uintptr_t* ent, Map req)
  {
    using namespace util;
    Expects(ent != nullptr);
    Expects(within_range(req.lin));

    // Map locally if all local requirements are met
    if (!is_page_dir(*ent) and req.size >= page_size
        and (req.page_sizes & page_size)
        and is_page_aligned(req.lin) and is_page_aligned(req.phys)) {

      auto res = map_entry(ent, req);
      Ensures(res and res.size == page_size);
      return res;
    }

    // If request is an unmap, clear entry, deallocating any allocated page_dir.
    bool is_unmap = req.phys == 0 and req.flags == Pflag::none;
    if (is_unmap) {
      Expects(bits::is_aligned(Arch::min_pagesize, req.size));

      if (is_page_dir(*ent)) {
        auto* pdir = page_dir(ent);
        if (req.size >= page_size) {
          // Delete this page dir only if it's completely covered by the request
          // E.g. Keep it if other mapping with lower page sizes might touch it.
          // TODO: Consider checking if the directory is in use, to trigger
          //       deallocations more frequently.
          delete pdir;
        } else {
          return pdir->map_r(req);
        }
      } else {
        // NOTE: Mapping the 0-page to 0 after id-mapping lower memory implies
        //       unmap, which again might require creation of new tables.
      }

      // Clear the entry if it's definitely not in use after unmap.
      if (req.size >= page_size) {
        *ent = 0;
        req.size = page_size;
        return req;
      }
    }

    // If minimum requested page size was not smaller than this lvl psize, fail
    if (req.min_psize() >= page_size)
      return Map();

    // Mapping via sub directory, creating subdir if needed.
    if (!is_page_dir(*ent)) {
      // Fragment current entry into a new page dir.

      // The requested mapping might not be to the beginning of this entry
      auto aligned_addr  = req.lin & ~(page_size - 1);

      // Create new page dir, inheriting physical addr and flags from this entry
      auto current_addr  = addr_of(*ent);
      auto current_flags = flags_of(*ent);
      create_page_dir(aligned_addr, current_addr, current_flags);
    }

    Ensures(is_page_dir(*ent));
    permit_flags(ent, req.flags);

    auto* pdir = page_dir(ent);
    Expects(pdir != nullptr);

    PG_PRINT("<map_entry_r> Sub 0x%p want: %s\n",  pdir, req.to_string().c_str());

    auto res = pdir->map_r(req);

    // We either get no result or a partial / correct one
    if (res)
    {
      Ensures(res.size <= page_size);
      Ensures((req.flags & res.flags) == req.flags);
    }
    else
    {
      // If result is empty we had page size constraints that couldn't be met
      auto sub_psize = pdir->page_size;
      Ensures((req.page_sizes & page_size) == 0
              or (pdir->is_page_dir(req.lin) and (req.page_sizes & sub_psize))
              or (sub_psize & req.page_sizes) == 0);
    }

    PG_PRINT("<map_entry_r> Sub 0x%p got: %s\n",  pdir, res.to_string().c_str());

    return res;
  }

  /**
   * Recursively map linear address to phys, setting provided flags.
   * Different page sizes may be used, and new page directories may be allocated
   * to match size as closely as possible while minimizing page count.
   **/
  Map map_r(Map req)
  {
    using namespace util;
    PG_PRINT("<map_r> %s\n", req.to_string().c_str());
    Expects(req);
    Expects(within_range(req.lin));

    Expects(bits::is_aligned<min_pagesize>(req.lin));
    Expects(bits::is_aligned(req.min_psize(), req.lin));
    Expects(req.lin < Arch::max_memory);

    if (req.phys != any_addr) {
      Expects(bits::is_aligned<min_pagesize>(req.phys));
      Expects(bits::is_aligned(req.min_psize(), req.phys));
      Expects(req.phys < Arch::max_memory);
    }

    Expects((req.page_sizes & os::mem::supported_page_sizes()) != 0);

    Map res{};

    for (auto i = tbl_.begin() + indexof(req.lin); i != tbl_.end(); i++)
    {
      auto* ent = entry(req.lin + res.size);

      Map sub {req.lin + res.size, req.phys == any_addr ? any_addr : req.phys + res.size, req.flags,
          req.size - res.size, req.page_sizes};

      res += map_entry_r(ent, sub);

      if (! res)
        return res;

      if (res.size >= req.size)
        break;
    }

    Ensures(res);
    Ensures((req.page_sizes & res.page_sizes) != 0);
    Ensures(res.size <= util::bits::roundto<4_KiB>(req.size));
    Ensures(res.lin == req.lin);
    if (req.phys != any_addr)
      Ensures(res.phys == req.phys);


    if (res.phys == any_addr) {
      // TODO: avoid traversing the tables again to get physical addr.
      // For now we're using response maps as requests in subsequent calls
      // so we can't set it to res.phys deeper in.
      res.phys = addr_of(*entry_r(req.lin));
    }
    return res;
  }


  /** Recursively get protection flags for a page enclosing addr **/
  Pflag flags_r(uintptr_t addr)
  {
    auto* ent = entry_r(addr);

    if (ent == nullptr)
      return Pflag::none;

    return flags_of(*ent);
  }


  /** CPU entry point for the internal data structures **/
  void* data()
  { return &tbl_; }


  /** Get table entry at index **/
  uintptr_t& at(int i)
  { return tbl_.at(i); }


  void traverse(delegate<void(void*, size_t)> callback)
  {
    callback(this, sizeof(Page_table));
    for (auto& ent : tbl_)
      if(is_page_dir(ent)) {
        auto* pdir = page_dir(&ent);
        pdir->traverse(callback);
      }
  }

private:
  alignas(Arch::min_pagesize) std::array<uintptr_t, Arch::table_size> tbl_ {};
  const uintptr_t linear_addr_start_ = 0;
};


//
// Page table types for 4-level x86 paging
//
using Pml1 = Page_table<4_KiB, void, x86_64, Flags::all & ~(Flags::huge | Flags::pdir)>;
using Pml2 = Page_table<Pml1::range_size, Pml1>;
using Pml3 = Page_table<Pml2::range_size, Pml2>;
using Pml4 = Page_table<Pml3::range_size, Pml3, x86_64, Flags::all & ~Flags::huge>;


//
// Specializations for lowest level 4k page tables (e.g. leaf nodes)
//

template <>
inline bool Pml1::is_page_dir(uintptr_t) noexcept
{ return false; }

template <>
inline uintptr_t* Pml1::entry(uintptr_t addr){
  auto index = indexof(addr);

  if (index < 0) return nullptr;

  return &tbl_.at(index);
}


template <>
inline uintptr_t* Pml1::entry_r(uintptr_t addr){
  return entry(addr);
}


template <>
inline Flags Pml1::set_flags_r(uintptr_t addr, Pflag flags){
  return set_flags(addr, flags);
}


template <>
inline void* Pml1::page_dir(uintptr_t*){
  return nullptr;
}


template <>
inline uintptr_t Pml1::active_page_size(uintptr_t addr) {
  auto* entry_ = entry(addr);
  if (entry_ == nullptr) {
    return 0;
  }

  return page_size;
}



template <>
inline  x86::paging::Summary Pml1::summary(bool, int){
  Summary sum;
  for (auto ent : tbl_)
  {
    if (util::has_flag(flags_of(ent), Pflag::present))
    {
      sum.pages_4k++;
    }
  }
  return sum;
}


template <>
inline Map Pml1::map_r(Map req)
{
  PG_PRINT("<map_r> mapping 0x%lx -> 0x%lx, size %li \n", req.lin, req.phys, req.size);
  Expects(req);

  if (req.size == 0) {
    PG_PRINT("<map> pml1 asked for 0 size: %s\n", req.to_string().c_str());
    return Map();
  }

  if ((req.page_sizes & page_size) == 0) {
    PG_PRINT("<map> pml1 asked for different page sizes: 0x%lx\n", req.page_sizes);
    return Map();
  }

  auto res = map(req);
  Expects(res.size);
  Ensures(res.lin == req.lin);
  return res;
}


/** Invalidate page (e.g. flush TLB entry) **/
void invalidate(void *pageaddr);

template <>
inline void Pml1::traverse(delegate<void(void*, size_t)>) {}

} // namespace paging
} // namespace x86

#endif
