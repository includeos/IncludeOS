
/*
 * Self sufficient Utility function that will copy a binary to a certain
 * location and then jump to it. The idea (from fwsgonzo) is to have this
 * function copied to an otherwise unused place in memory so that we can
 * overwrite the currently running binary with a new one.
 */
#include <cstdint>
asm(".org 0x2000");

extern "C" __attribute__((noreturn))
void hotswap(const char* base, int len, char* dest, void* start,
             uintptr_t magic, uintptr_t bootinfo)
{
  // Copy binary to its destination
  for (int i = 0; i < len; i++)
    dest[i] = base[i];

  // Populate multiboot regs and jump to start
  asm volatile("jmp *%0" : : "r" (start), "a" (magic), "b" (bootinfo));
  asm volatile (".global __hotswap_end;\n__hotswap_end:");
  __builtin_unreachable();
}
