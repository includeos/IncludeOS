#include "modes.hpp"

// user modes
mode_table_t usermodes("airoOws");
// channel modes
mode_table_t chanmodes("eIbklimnpstu");

uint16_t default_user_modes()
{
  return 2; // +i
}
std::string default_channel_modes()
{
  return "nt";
}
