
#pragma once
#ifndef KERNEL_ELF_HPP
#define KERNEL_ELF_HPP

#include <cstdint>
#include <string>
#include <vector>

struct safe_func_offset {
  const char* name;
  uintptr_t   addr;
  uint32_t    offset;
};

struct Elf
{
  static uintptr_t resolve_addr(uintptr_t addr);
  static uintptr_t resolve_addr(void* addr);

  // doesn't use heap
  static safe_func_offset
    safe_resolve_symbol(const void* addr, char* buffer, size_t length);

  //returns the address of a symbol, or 0
  static uintptr_t
    resolve_name(const std::string& name);

  // returns the address of the first byte after the ELF header
  // and all static and dynamic sections
  static size_t end_of_file();

  // debugging purposes only
  static const char* get_strtab();
  static void        print_info();
  static bool        verify_symbols();
};

#endif
