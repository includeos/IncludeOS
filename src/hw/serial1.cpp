#include <hw/serial.hpp>

void hw::Serial::print1(const char* cstr) {
  char* p = (char*)cstr;
  auto port_ = ports_[0];
  while (*p) {
    while (not (hw::inb(port_ + 5) & 0x20));
    hw::outb(port_, *p++);
  }
}
