#include "storage.hpp"

#include <cassert>

storage_header::storage_header()
{
  this->magic = 0xbaadb33f;
}

storage_entry::storage_entry(int t, const std::string& n, int len)
  : type(t), length(len)
{
  assert(type >= 0);
  assert(length >= 0);
  int written = snprintf(name, sizeof(name), "%s", n.c_str());
  assert(written > 0);
}

std::string storage_entry::to_string() const
{
  switch (type) {
  case 0:
    return "STRING(" + std::string(name) + ") == '" + std::string(vla, length) + "'";
  case 1:
    return "CONNECTION(" + std::string(name) + ") len=" + std::to_string(length);
  default:
    assert(0 && "Unknown type");
  }
}

