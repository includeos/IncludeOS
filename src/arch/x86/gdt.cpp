#include "gdt.hpp"

#include <cstring>
#include <cassert>
#include <new>

#define ACCESS_DUMMY  0x0
#define ACCESS_CODE   0x9A
#define ACCESS_DATA   0x92
#define ACCESS_CODE3  0xFA
#define ACCESS_DATA3  0xF2

#define FLAGS_X32_PAGE 0xC

namespace x86
{
extern "C" void __load_gdt(void*);

void GDT::reload_gdt(GDT& table) noexcept
{
  __load_gdt(&table.desc);
}

void GDT::initialize() noexcept
{
  new(this) struct GDT();

  // null entry
  auto& nullent = this->create();
  __builtin_memset(&nullent, 0, sizeof(nullent));

  // code (ring0)
  auto& code = this->create();
  code.limit_lo = 0xffff;
  code.base_lo  = 0;
  code.access   = ACCESS_CODE;
  code.limit_hi = 0xf;
  code.flags    = FLAGS_X32_PAGE;
  code.base_hi  = 0;

  // data (ring0)
  auto& data = this->create();
  data.limit_lo = 0xffff;
  data.base_lo  = 0;
  data.access   = ACCESS_DATA;
  data.limit_hi = 0xf;
  data.flags    = FLAGS_X32_PAGE;
  data.base_hi  = 0;
}

int GDT::create_data(void* ptr, uint16_t pages) noexcept
{
  uintptr_t base = (uintptr_t) ptr;
  assert((base & 0x3f) == 0); // at least 64-byte aligned
  this->create_data(base, pages);
  return this->count-1;
}

gdt_entry& GDT::create_data(uint32_t base, uint16_t size) noexcept
{
  auto& ent = create();
  ent.limit_lo = size;
  ent.base_lo  = base & 0xffffff;
  ent.access   = ACCESS_DATA;
  ent.limit_hi = 0x0;
  ent.flags    = FLAGS_X32_PAGE;
  ent.base_hi  = (base >> 24) & 0xff;
  return ent;
}

} // x86
