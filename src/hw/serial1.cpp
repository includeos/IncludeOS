#include <hw/serial.hpp>

static const uint16_t port = 0x3F8; // Serial 1

extern "C" {
#define UKVM_GUEST_PTR(T) T

/* UKVM_HYPERCALL_PUTS */
struct ukvm_puts {
    /* IN */
    UKVM_GUEST_PTR(const char *) data;
    size_t len;
};

#define UKVM_HYPERCALL_PIO_BASE 0x500

  static inline void ukvm_do_hypercall(int n, volatile void *arg)
  {
    __asm__ __volatile__("outl %0, %1"
            :
            : "a" ((uint32_t)((uint64_t)arg)),
              "d" ((uint16_t)(UKVM_HYPERCALL_PIO_BASE + n))
            : "memory");
  }

enum ukvm_hypercall {
    /* UKVM_HYPERCALL_RESERVED=0 */
    UKVM_HYPERCALL_WALLTIME=1,
    UKVM_HYPERCALL_PUTS,
    UKVM_HYPERCALL_POLL,
    UKVM_HYPERCALL_BLKINFO,
    UKVM_HYPERCALL_BLKWRITE,
    UKVM_HYPERCALL_BLKREAD,
    UKVM_HYPERCALL_NETINFO,
    UKVM_HYPERCALL_NETWRITE,
    UKVM_HYPERCALL_NETREAD,
    UKVM_HYPERCALL_MAX
};

int platform_puts(const char *buf, int n)
{
    struct ukvm_puts str;

    str.data = (char *)buf;
    str.len = n;

    ukvm_do_hypercall(UKVM_HYPERCALL_PUTS, &str);

    return str.len;
}

extern "C"
bool use_solo5 = true;

extern "C"
void __serial_print1(const char* cstr)
{
  if (use_solo5) {
    platform_puts(cstr, strlen(cstr));
  } else {
    while (*cstr) {
      while (not (hw::inb(port + 5) & 0x20));
        hw::outb(port, *cstr++);
    }
  }
}

extern "C"
void __init_serial_solo5()
{
  use_solo5 = true;
}

extern "C"
void __init_serial1()
{
  // properly initialize serial port
  hw::outb(port + 1, 0x00);    // Disable all interrupts
  hw::outb(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
  hw::outb(port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
  hw::outb(port + 1, 0x00);    //                  (hi byte)
  hw::outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
  hw::outb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold

  use_solo5 = false;
}

}
