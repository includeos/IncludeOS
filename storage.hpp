#pragma once
#include <cstdint>
#include <string>

enum storage_type
{
  TYPE_END    = 0,
  TYPE_STRING = 1,
  TYPE_TCP    = 2,
  TYPE_UDP    = 3,
};

struct storage_entry
{
  storage_entry(int type, const std::string& name, int len);
  
  int  type;
  char name[64];
  int  length = 0;
  
  char vla[0];

  std::string to_string() const;
};

struct storage_header
{
  storage_header();
  
  template <typename... Args>
  storage_entry& create_entry(Args&&... args);
  
  
  char* get_entry(int);
  char* get_entry(const char*);
  
  int64_t  magic;
  uint32_t entries = 0;
  uint32_t length  = 0;
  char     vla[0];
};

template <typename... Args>
inline storage_entry&
storage_header::create_entry(Args&&... args)
{
  // create entry
  auto* entry = (storage_entry*) &vla[length];
  new (entry) storage_entry(args...);
  // next storage_entry will be this much further out:
  this->length += entry->length;
  this->entries++;

  return *entry;
}
