
#include <statman>
#include <info>
#include <smp_utils>

// this is done to make sure construction only happens here
static Statman statman_instance;
Statman& Statman::get() {
  return statman_instance;
}

Stat::Stat(const Stat_type type, const std::string& name)
  : ui64(0)
{
  // stats are persisted by default
  this->m_bits = uint8_t(type) | PERSIST_BIT;

  if (name.size() > MAX_NAME_LEN)
    throw Stats_exception("Stat name cannot be longer than " + std::to_string(MAX_NAME_LEN) + " characters");

  snprintf(name_, sizeof(name_), "%s", name.c_str());
}
Stat::Stat(const Stat& other) {
  this->ui64   = other.ui64;
  this->m_bits = other.m_bits;
  __builtin_memcpy(this->name_, other.name_, sizeof(name_));
}
Stat& Stat::operator=(const Stat& other) {
  this->ui64   = other.ui64;
  this->m_bits = other.m_bits;
  __builtin_memcpy(this->name_, other.name_, sizeof(name_));
  return *this;
}

void Stat::operator++() {
  switch (this->type()) {
    case UINT32: ui32++;    break;
    case UINT64: ui64++;    break;
    case FLOAT:  f += 1.0f; break;
    default: throw Stats_exception("Invalid stat type encountered when incrementing");
  }
}

std::string Stat::to_string() const {
  switch (this->type()) {
    case UINT32: return std::to_string(ui32);
    case UINT64: return std::to_string(ui64);
    case FLOAT:  return std::to_string(f);
    default:     return "Unknown stat type";
  }
}

///////////////////////////////////////////////////////////////////////////////

Statman::Statman() {
  this->create(Stat::UINT32, "statman.unused_stats");
}

Stat& Statman::create(const Stat::Stat_type type, const std::string& name)
{
  stlock.lock();
  if (name.empty()) {
    stlock.unlock();
    throw Stats_exception("Cannot create Stat with no name");
  }

  const ssize_t idx = this->find_free_stat();
  if (idx < 0) {
    // FIXME: this can throw, and leave the spinlock unlocked
    m_stats.emplace_back(type, name);
    auto& retval = m_stats.back();
    stlock.unlock();
    return retval;
  }

  // note: we have to create this early in case it throws
  auto& stat = *new (&m_stats[idx]) Stat(type, name);
  unused_stats()--; // decrease unused stats
  stlock.unlock();
  return stat;
}

Stat& Statman::get(const Stat* st)
{
  stlock.lock();
  for (auto& stat : this->m_stats) {
    if (&stat == st) {
      if (stat.unused() == false) {
          stlock.unlock();
          return stat;
      }
      stlock.unlock();
      throw Stats_exception("Accessing deleted stat");
    }
  }
  stlock.unlock();
  throw std::out_of_range("Not a valid stat in this statman instance");
}

Stat& Statman::get_by_name(const char* name)
{
  /// FIXME FIXME FIXME ///
  ///  Regression here  ///
  /// FIXME FIXME FIXME ///
  
  //printf("get_by_name Locking: this=%p, lock=%p, %d\n", this, &stlock, *(spinlock_t*) &stlock);
  //stlock.lock();
  for (auto& stat : this->m_stats)
  {
    if (stat.unused() == false) {
      if (strncmp(stat.name(), name, Stat::MAX_NAME_LEN) == 0)
        //stlock.unlock();
        printf("Unlocked (found): this=%p, lock=%p, %d\n", this, &stlock, *(spinlock_t*) &stlock);
        return stat;
    }
  }
  //stlock.unlock();
  //printf("Unlocked (not found): %d\n", *(spinlock_t*) &stlock);
  throw std::out_of_range("No stat found with exact given name");
}

Stat& Statman::get_or_create(const Stat::Stat_type type, const std::string& name)
{
  try {
    auto& stat = get_by_name(name.c_str());
    if(type == stat.type())
      return stat;
  }
  catch(const std::exception&) {
    return create(type, name);
  }

  throw Stats_exception("Mismatch between stat type");
}

void Statman::free(void* addr)
{
  auto& stat = this->get((Stat*) addr);
  stlock.lock();
  // delete entry
  new (&stat) Stat(Stat::FLOAT, "");
  unused_stats()++; // increase unused stats
  stlock.unlock();
}

ssize_t Statman::find_free_stat() const noexcept
{
  for (size_t i = 0; i < this->m_stats.size(); i++)
  {
    if (m_stats[i].unused()) return i;
  }
  return -1;
}

void Statman::clear()
{
  if (size() <= 1) return;
  m_stats.clear();
  this->create(Stat::UINT32, "statman.unused_stats");
}
