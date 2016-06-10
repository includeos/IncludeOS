#pragma once

#include <cstdint>
#include <string>

struct umode_t 
{
  const char value;
};
extern const std::string umode_string;

#define UMODE_IRCOP_MASK    (1 << 0)
