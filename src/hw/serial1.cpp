#include <hw/serial.hpp>

extern "C"
void __serial_print1(const char* cstr)
{
  const uint16_t port = 0x3F8; // Serial 1
  static bool init = false;
  if (!init) {
    init = true;
    // properly initialize serial port
    hw::outb(port + 1, 0x00);    // Disable all interrupts
    hw::outb(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    hw::outb(port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    hw::outb(port + 1, 0x00);    //                  (hi byte)
    hw::outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
    hw::outb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
  }
  
  while (*cstr) {
    while (not (hw::inb(port + 5) & 0x20));
    hw::outb(port, *cstr++);
  }
}
