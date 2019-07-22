/**
 * Master thesis
 * by Alf-Andre Walla 2016-2017
 *
**/
#ifdef __includeos__
asm(".org 0x8000");
#define HOTS_ATTR __attribute__((noreturn))
#else
#define HOTS_ATTR /* */
#endif
#define SOFT_RESET_MAGIC   0xFEE1DEAD

extern "C" HOTS_ATTR
void hotswap(char* dest, const char* base, int len,
             void* start, void* reset_data)
{
  // remainder
  for (int i = 0; i < len; i++)
    dest[i] = base[i];

#ifdef __includeos__
  // jump to _start
  asm volatile("jmp *%2" : : "a" (SOFT_RESET_MAGIC), "b" (reset_data), "c" (start));
  asm volatile(
  ".global __hotswap_length;\n"
  "__hotswap_length:" );
  // we can never get here!
  __builtin_unreachable();
#endif
}
