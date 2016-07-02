#include "modes.hpp"

// user modes
mode_table_t usermodes("airoOws");
// channel modes
mode_table_t chanmodes("ntsplkbIeu");

uint16_t default_user_modes()
{
  return 2; // +i
}
uint16_t default_channel_modes()
{
  return 1 | 2; // +nt
}
