#include <hw/serial.hpp>

extern "C"
void __serial_print1(const char* cstr) {
  hw::Serial::print1(cstr);
}

void hw::Serial::print1(const char* str) {
  auto port_ = ports_[0];
  while (*str) {
    while (not (hw::inb(port_ + 5) & 0x20));
    hw::outb(port_, *str++);
  }
}
