
#include <util/alloc_pmr.hpp>

std::pmr::memory_resource* std::pmr::get_default_resource() noexcept {
  static os::mem::Default_pmr* default_pmr;
  if (default_pmr == nullptr)
    default_pmr = new os::mem::Default_pmr{};
  return default_pmr;
}
