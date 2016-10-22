asm(".org 0x8000");
extern "C"
void hotswap(const char* base, int len, char* dest, void* start)
{
  // replace kernel
  for (int i = 0; i < len; i++)
    dest[i] = base[i];
  // jump to _start
  asm volatile("jmp *%0" : : "r" (start));
  asm volatile(
  ".global __hotswap_length;\n"
  "__hotswap_length:" );
}
