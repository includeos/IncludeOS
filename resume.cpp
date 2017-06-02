/**
 * Master thesis
 * by Alf-Andre Walla 2016-2017
 *
**/
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
  auto num_ents = storage.get_entries();
  if (num_ents > 1)
      printf("* Resuming %d stored entries\n", num_ents-1);
  else
      printf("* No stored entries to resume\n");

  for (auto* ptr = storage.begin(); ptr->type != TYPE_END;)
  {
    auto* oldptr = ptr;
    // resume wrapper
    Restore wrapper {ptr};
    // use registered functions when we can, otherwise, use normal
    auto it = resume_funcs.find(ptr->id);
    if (it != resume_funcs.end())
    {
      it->second(wrapper);
    } else {
      func(wrapper);
    }
    // if we are already at the end due calls to go_next, break early
    if (ptr->type == TYPE_END) break;
    // call next manually only when no one called go_next
    if (oldptr == ptr) ptr = storage.next(ptr);
  }
  /// wake all the slumbering IP stacks
  serialized_tcp::wakeup_ip_networks();
  /// zero out all the state for security reasons
  storage.zero();

  return true;
}

void LiveUpdate::on_resume(uint16_t id, resume_func func)
{
  resume_funcs[id] = func;
}

/// struct Restore

bool        Restore::is_marker() const noexcept
{
  return ent->type == TYPE_MARKER;
}
int         Restore::as_int()    const
{
  // this type uses the length field directly as value to save space
  if (ent->type == TYPE_INTEGER)
      return ent->len;
  throw std::runtime_error("Incorrect type: " + std::to_string(ent->type));
}
std::string Restore::as_string() const
{
  if (ent->type == TYPE_STRING)
      return std::string(ent->data(), ent->len);
  throw std::runtime_error("Incorrect type: " + std::to_string(ent->type));
}
buffer_t  Restore::as_buffer() const
{
  if (ent->type == TYPE_BUFFER) {
      buffer_t buffer;
      buffer.assign(ent->data(), ent->data() + ent->len);
      return buffer;
  }
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
  if (size != segs.esize)
      throw std::runtime_error("Incorrect type size: " + std::to_string(size) + " vs " + std::to_string(segs.esize));

  count = segs.count;
  return (const void*) segs.vla;
}
std::vector<std::string> Restore::rebuild_string_vector() const
{
  if (ent->type != TYPE_STR_VECTOR)
      throw std::runtime_error("Incorrect type: " + std::to_string(ent->type));
  std::vector<std::string> retv;
  // reserve just enough room
  auto* begin = (varseg_begin*) ent->vla;
  retv.reserve(begin->count);

  auto* el = (varseg_entry*) begin->vla;
  for (size_t i = 0; i < begin->count; i++)
  {
    //printf("Restoring string len=%u: %.*s\n", el->len, el->len, el->vla);
    retv.emplace_back(el->vla, el->len);
    // next
    el = (varseg_entry*) &el->vla[el->len];
  }
  return retv;
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

void Restore::pop_marker()
{
  while (is_marker() == false
      && is_end() == false) go_next();
}
void Restore::pop_marker(uint16_t id)
{
  while (not (is_marker() && get_id() == id)
          && is_end() == false) go_next();
}

void Restore::cancel()
{
  while (is_end() == false) go_next();
}

// copy operator
Restore::Restore(const Restore& other)
  : ent(other.ent)  {}

}
