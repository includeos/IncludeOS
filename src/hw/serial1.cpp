#include <hw/serial.hpp>

extern "C"
void __serial_print1(const char* cstr)
{
  const uint16_t port = 0x3F8; // Serial 1
  while (*cstr) {
    while (not (hw::inb(port + 5) & 0x20));
    hw::outb(port, *cstr++);
  }
}
