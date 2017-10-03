#include "modes.hpp"
#include "common.hpp"

// visible user modes
mode_table_t usermodes("airoOws");
// visible channel modes (+eIb)
mode_table_t chanmodes("klimnpstu");

mode_table_t::mode_table_t(const char* lut)
  : LUT(lut) {}

uint16_t default_user_modes()
{
  return 2; // +i
}
uint16_t default_channel_modes()
{
  return 16 | 128; // +nt
}
