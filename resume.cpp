#include "liveupdate.hpp"

#include <cstdio>
#include "storage.hpp"
#include "serialize_tcp.hpp"
#include <map>

namespace liu
{
static std::map<uint16_t, LiveUpdate::resume_func> resume_funcs;

bool resume_begin(storage_header& storage, LiveUpdate::resume_func func)
{
  /// restore each entry one by one, calling registered handlers
  printf("* Resuming %d stored entries\n", storage.get_entries());
  
  for (auto* ptr = storage.begin(); ptr->type != TYPE_END; ptr = storage.next(ptr))
  {
    // use registered functions when we can, otherwise, use normal
    auto it = resume_funcs.find(ptr->id);
    if (it != resume_funcs.end())
    {
      it->second(ptr);
    } else {
      func(ptr);
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
  if (ent->type == TYPE_STRING)
      return std::string(ent->data(), ent->len);
  throw std::runtime_error("Incorrect type: " + std::to_string(ent->type));
}
buffer_len  Restore::as_buffer() const
{
  if (ent->type == TYPE_BUFFER)
      return {ent->data(), ent->len};
  throw std::runtime_error("Incorrect type: " + std::to_string(ent->type));
}
Restore::Connection_ptr Restore::as_tcp_connection(net::TCP& tcp) const
{
  return deserialize_connection(ent->vla, tcp);
}

int16_t     Restore::get_type() const noexcept
{
  return ent->type;
}
uint16_t    Restore::get_id() const noexcept
{
  return ent->id;
}
int32_t     Restore::length() const noexcept
{
  return ent->len;
}
const void* Restore::data() const noexcept
{
  return ent->data();
}
const void* Restore::get_segment(size_t size, size_t& count) const
{
  if (ent->type != TYPE_VECTOR)
      throw std::runtime_error("Incorrect type: " + std::to_string(ent->type));

  auto& segs = ent->get_segs();
  if (ent->type != TYPE_VECTOR)
      throw std::runtime_error("Incorrect T size: " + std::to_string(size) + " vs " + std::to_string(segs.esize));
  
  count = segs.count;
  return (const void*) segs.vla;
}

/// 
bool     Restore::is_end() const noexcept
{
  return get_type() == TYPE_END;
}
void     Restore::go_next()
{
  if (is_end())
      throw std::runtime_error("Already at end of entries");
  // increase the counter, so the resume loop skips entries properly
  ent = ent->next();
}
uint16_t Restore::next_id() const noexcept
{
  return ent->next()->id;
}

// copy operator
Restore::Restore(const Restore& other)
  : ent(other.ent)  {}

}
