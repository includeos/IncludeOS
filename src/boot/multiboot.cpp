#include <cstdint>
#include <boot/multiboot.h>

// Let the linker provide as much info as possible
extern uintptr_t _MULTIBOOT_START_;
extern uintptr_t _LOAD_START_;
extern uintptr_t _LOAD_END_;
extern uintptr_t _BSS_END_;
extern uintptr_t _TEXT_START_;
extern uintptr_t _start;

#define MULTIBOOT_HEADER_FLAGS  MULTIBOOT_MEMORY_INFO

extern "C" volatile const multiboot_header __multiboot__ {
  // magic
  MULTIBOOT_HEADER_MAGIC,
  // flags
    MULTIBOOT_HEADER_FLAGS,
  // checksum
    (uint32_t) 0 - (MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS),
  // header_addr
    (uint32_t)&_MULTIBOOT_START_,
  // load_addr
    (uint32_t)&_LOAD_START_,
  // load_end_addr
    (uint32_t)&_LOAD_END_,
  // bss_end_addr
    (uint32_t)&_BSS_END_,
  // entry_addr
    (uint32_t)&_start,

  // Video mode
  // mode_type
    0, // Unused
  // width
    100, // Unused
  // height
    50, // Unused
  // depth
    1

};
