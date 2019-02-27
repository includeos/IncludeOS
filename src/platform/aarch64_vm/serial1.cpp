#include <hw/serial.hpp>

#include <stdarg.h>
//static const uint16_t port = 0x3F8; // Serial 1
static char initialized __attribute__((section(".data"))) = 0x0;

static const uint32_t UART_BASE=0x09000000;

//__attribute__((no_sanitize("all")))
static inline void init_if_needed()
{
  if (initialized == true) return;
  initialized = true;


  /* init UART (38400 8N1) */ //maybe stick in asm as a function instead
  /*asm volatile("ldr	x4, =%0" :: "r"(UART_BASE));		// UART base
  asm volatile("mov	w5, #0x10");
  asm volatile("str	w5, [x4, #0x24]");
  asm volatile("mov	w5, #0xc300");
  asm volatile("orr	w5, w5, #0x0001");
  asm volatile("str	w5, [x4, #0x30]");*/
  //unsure if this becomes sane or not. look at asm but should be exactly the same as the above comment
  *((volatile unsigned int *) UART_BASE+0x24) = 0x10;
  *((volatile unsigned int *) UART_BASE+0x30) = 0xC301;
}

extern "C"
//__attribute__((no_sanitize("all")))
void __serial_print1(const char* cstr)
{
  init_if_needed();
  while (*cstr) {
    //No check what so ever probably not ok
    *((volatile unsigned int *) UART_BASE) = *cstr++;
  /*  while (not (hw::inb(port + 5) & 0x20));
    hw::outb(port, *cstr++);*/
  }
}
extern "C"
void __serial_print(const char* str, size_t len)
{
  init_if_needed();
  for (size_t i = 0; i < len; i++) {
    *((unsigned int *) UART_BASE) = str[i];
    /*while (not (hw::inb(port + 5) & 0x20));
    hw::outb(port, str[i]);*/
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
