#include "update.hpp"

#include <cstdio>
#include "storage.hpp"

static std::map<uint16_t, LiveUpdate::resume_func> resume_funcs;

bool resume_begin(storage_header& storage, LiveUpdate::resume_func func)
{
  /// verify checksum
  
  /// restore each entry one by one, calling registered handlers
  printf("* Resuming %d stored entries\n", storage.get_entries());
  
  for (auto* ptr = storage.begin(); ptr->type != TYPE_END; ptr = storage.next(ptr))
  {
    // use registered functions when we can, otherwise, use normal
    auto it = resume_funcs.find(ptr->id);
    if (it != resume_funcs.end())
    {
      it->second(*ptr);
    } else {
      func(*ptr);
    }
  }
  /// zero out all the values for security reasons
  storage.zero();
  
  return true;
}

void LiveUpdate::on_resume(uint16_t id, resume_func func)
{
  resume_funcs[id] = func;
}

/// struct Restore

std::string Restore::as_string() const
{
  if (ent.type == TYPE_STRING)
      return std::string(ent.vla, ent.len);
  throw std::runtime_error("Incorrect type: " + std::to_string(ent.type));
}
buffer_len  Restore::as_buffer() const
{
  if (ent.type == TYPE_BUFFER)
      return {ent.vla, ent.len};
  throw std::runtime_error("Incorrect type: " + std::to_string(ent.type));
}
#include "serialize_tcp.hpp"
Restore::Connection Restore::as_tcp_connection(net::TCP& tcp) const
{
  return deserialize_connection(ent.vla, tcp);
}

int16_t  Restore::get_type() const noexcept
{
  return ent.type;
}
uint16_t Restore::get_id() const noexcept
{
  return ent.id;
}
int32_t  Restore::length() const noexcept
{
  return ent.len;
}
void*    Restore::data() const noexcept
{
  return ent.vla;
}
