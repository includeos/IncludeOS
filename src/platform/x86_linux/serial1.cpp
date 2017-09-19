#include <cstring>
#include <unistd.h>

extern "C"
void __serial_print1(const char* cstr)
{
  size_t len = strlen(cstr);
  write(0, cstr, len);
}
extern "C"
void __serial_print(const char* str, size_t len)
{
  write(0, str, len);
}
