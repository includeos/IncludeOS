#include "storage.hpp"

#include <cassert>

storage_header::storage_header(uint64_t value)
  : magic(value)
{
  printf("%p --> %#llx\n", this, value);
}

void storage_header::add_string(uint16_t id, const std::string& data)
{
  auto& entry = create_entry(TYPE_STRING, id, data.size()+1);
  /// create string
  sprintf(entry.vla, "%s", data.c_str());
}
void storage_header::add_buffer(uint16_t id, const char* buffer, int length)
{
  auto& entry = create_entry(TYPE_BUFFER, id, length);
  memcpy(entry.vla, buffer, length);
}
void storage_header::add_end()
{
  create_entry(TYPE_END);
}

storage_entry* storage_header::begin()
{
  return (storage_entry*) vla;
}
storage_entry* storage_header::next(storage_entry* ptr)
{
  assert(ptr);
  return (storage_entry*) &ptr->vla[ptr->len];
}

storage_entry::storage_entry(int16_t t, uint16_t ID, int l)
  : type(t), id(ID), len(l)
{
  assert(type >= 0);
  assert(len >= 0);
}
// used for last entry, for the most part
storage_entry::storage_entry(int16_t t)
  : type(t), id(0), len(0)  {}
