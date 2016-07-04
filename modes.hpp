#pragma once

#include <cassert>
#include <cstdint>
#include <string>

struct mode_table_t
{
  mode_table_t(const std::string& lut)
    : LUT(lut) {}
  
  bool is_mode(char c) {
    return LUT.find(c) != std::string::npos;
  }
  int char_to_bit(char c) {
    size_t bit = LUT.find(c);
    return (bit != std::string::npos) ? bit : -1;
  }
  char bit_to_char(uint16_t bit) {
    assert(bit >= 0 && bit < LUT.size());
    return LUT[bit];
  }
  
  const std::string& get() const { return LUT; }
  
  const std::string LUT;
};
extern mode_table_t usermodes;
extern mode_table_t chanmodes;


#define UMODE_AWAY      'a'
#define UMODE_INVIS     'i'
#define UMODE_RESTRICT  'r'
#define UMODE_IRCOP     'o'
#define UMODE_LIRCOP    'O'
#define UMODE_WALLOPS   'w'
#define UMODE_SNOTICE   's'

#define CMODE_NOEXTERN  'n'
#define CMODE_TOPIC     't'

extern uint16_t  default_user_modes();
extern uint16_t  default_channel_modes();
