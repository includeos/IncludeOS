#include "gdt.hpp"

#include <cstring>
#include <cassert>
#include <new>

#define ACCESS_DUMMY  0x0
#define ACCESS_CODE   0x9A
#define ACCESS_DATA   0x92
#define ACCESS_CODE3  0xFA
#define ACCESS_DATA3  0xF2

namespace x86
{
gdt_entry& gdt::create_data(uint32_t base, uint16_t size) {
  auto& ent = create();
  ent.limit_lo = size;
  ent.base_lo  = base & 0xffffff;
  ent.access   = ACCESS_DATA3;
  ent.limit_hi = 0x0;
  ent.flags    = 0xC;
  ent.base_hi  = (base >> 24) & 0xff;
  return ent;
}

extern "C" void __load_gdt(void*);

void GDT::initialize(gdt* table) noexcept
{
  new(table) struct gdt();

  // null entry
  auto& nullent = table->create();
  __builtin_memset(&nullent, 0, sizeof(nullent));

  // code (ring0)
  auto& code = table->create();
  code.limit_lo = 0xffff;
  code.base_lo  = 0;
  code.access   = ACCESS_CODE;
  code.limit_hi = 0xf;
  code.flags    = 0xC;
  code.base_hi  = 0;

  // data (ring0)
  auto& data = table->create();
  data.limit_lo = 0xffff;
  data.base_lo  = 0;
  data.access   = ACCESS_DATA;
  data.limit_hi = 0xf;
  data.flags    = 0xC;
  data.base_hi  = 0;

  // reload GDT & segment regs
  reload_gdt(table);
}

int GDT::create_data(gdt* table, uintptr_t base, uint16_t len) noexcept
{
  assert((base & 0xfffff000) == 0);
  table->create_data(base >> 12, len);
  return table->count-1;
}

void GDT::reload_gdt(gdt* table) noexcept
{
  __load_gdt(&table->desc);
}

} // x86
