#include <statman>
#include <liveupdate.hpp>

void Statman::store(uint32_t id, liu::Storage& store)
{
  store.add_buffer(id, this->cbegin(), this->num_bytes());
}
void Statman::restore(liu::Restore& store)
{
  auto buffer = store.as_buffer(); store.go_next();

  assert(buffer.size() % sizeof(Stat) == 0);
  const size_t count = buffer.size() / sizeof(Stat);

  const Stat* ptr = (Stat*) buffer.data();
  const Stat* end = ptr + count;

  for (; ptr < end; ptr++)
  {
    try {
      auto& stat = this->get_by_name(ptr->name());
      std::memcpy(&stat, ptr, sizeof(Stat));
    }
    catch (const std::exception& e)
    {
      auto& stat = this->create(ptr->type(), ptr->name());
      std::memcpy(&stat, ptr, sizeof(Stat));
    }
  }
}
