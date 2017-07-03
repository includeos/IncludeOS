#include <hw/serial.hpp>

extern "C" {
#include <solo5.h>
}

extern "C"
void __init_serial1() {}

extern "C"
void __serial_print1(const char* cstr)
{
  solo5_console_write(cstr, strlen(cstr));
}
