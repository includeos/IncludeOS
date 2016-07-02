#pragma once

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
    int bit = LUT.find(c)
    return (bit != std::string::npos) ? bit : -1;
  }
  char bit_to_char(int bit) {
    assert(bit >= 0 && bit < LUT.size());
    return LUT[bit];
  }
  
  const std::string LUT;
};
extern mode_table_t umodes;
extern mode_table_t cmodes;
