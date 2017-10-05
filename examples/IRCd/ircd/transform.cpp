#include <string>

static char ASCII_LUT[256];

void transform_init() {
  
  for (size_t i = 0; i < 256; i++) {
    ASCII_LUT[i] = std::toupper(i);
  }
  
}

void transform_to_upper(std::string& str)
{
  for (char& c : str) {
    c = ASCII_LUT[(unsigned) c];
  }
}
