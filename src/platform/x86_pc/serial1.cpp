#include <hw/serial.hpp>
#include <stdarg.h>
static const uint16_t port = 0x3F8; // Serial 1
static char initialized __attribute__((section(".data"))) = 0x0;

__attribute__((no_sanitize("all")))
static inline void init_if_needed()
{
  if (initialized == true) return;
  initialized = true;
  // properly initialize serial port
  hw::outb(port + 1, 0x00);    // Disable all interrupts
  hw::outb(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
  hw::outb(port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
  hw::outb(port + 1, 0x00);    //                  (hi byte)
  hw::outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
  hw::outb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
}

extern "C"
__attribute__((no_sanitize("all")))
void __serial_print1(const char* cstr)
{
  init_if_needed();
  while (*cstr) {
    while (not (hw::inb(port + 5) & 0x20));
    hw::outb(port, *cstr++);
  }
}
extern "C"
void __serial_print(const char* str, size_t len)
{
  init_if_needed();
  for (size_t i = 0; i < len; i++) {
    while (not (hw::inb(port + 5) & 0x20));
    hw::outb(port, str[i]);
  }
}

extern "C"
void kprint(const char* c){
  __serial_print1(c);
}

extern "C" void kprintf(const char* format, ...)
{
  char buf[8192];
  va_list aptr;
  va_start(aptr, format);
  vsnprintf(buf, sizeof(buf), format, aptr);
  __serial_print1(buf);
  va_end(aptr);
}
