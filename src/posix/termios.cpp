//#include <termios.h>
#include <cstdint>
#include <cstdio>

typedef unsigned long tcflag_t;
typedef unsigned long cc_t;
#define NCCS  64

struct termios
{
  tcflag_t  c_iflag;     // Input modes.
  tcflag_t  c_oflag;     // Output modes.
  tcflag_t  c_cflag;     // Control modes.
  tcflag_t  c_lflag;     // Local modes.
  cc_t      c_cc[NCCS];  // Control characters.
};

extern "C"
int tcsetattr(int fildes, int optional_actions, const struct termios *termios_p)
{
  printf("tcsetattr(%d, %d, %p)\n", fildes, optional_actions, termios_p);
}
extern "C"
int tcgetattr(int fildes, struct termios *termios_p)
{
  printf("tcgetattr(%d, %p)\n", fildes, termios_p);
}
