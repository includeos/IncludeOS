#include <kernel/memmap.hpp>

uintptr_t Memory_map::in_range(uintptr_t addr){

  if (UNLIKELY(map_.empty()))
    return 0;

  auto closest_above = map_.lower_bound(addr);

  // This might be an exact match to the beginning of the range
  if (UNLIKELY(closest_above->first == addr))
    return closest_above->first;

  if (closest_above == map_.end() or closest_above != map_.begin())
    closest_above --;

  if (closest_above->second.in_range(addr))
    return closest_above->first;

  // @note :  0 is never a valid range since a span can't start at address 0
  return 0;
}

Fixed_memory_range& Memory_map::assign_range (Fixed_memory_range&& rng) {

  debug("* Assgning range %s", rng.to_string().c_str());

  // Keys are address representations, not pointers
  auto key = reinterpret_cast<uintptr_t>(rng.cspan().data());

  if (UNLIKELY(map_.empty())) {
    auto new_entry  = map_.emplace(key, std::move(rng));
    debug("* Range inserted (success: 0x%x, %i)", new_entry.first->first, new_entry.second);
    return new_entry.first->second;
  }

  // Make sure the range does not overlap with any existing ranges
  auto closest_match = map_.lower_bound(key);

  // If the new range starts above all other ranges, we still need to make sure
  // the last element doesn't overlap
  if (closest_match == map_.end())
    closest_match --;

  if (UNLIKELY(rng.overlaps(closest_match->second)) )
    throw Memory_range_exception("Range '"+ std::string(rng.name())
                                 + "' overlaps with the range above key "
                                 + closest_match->second.to_string());

  // We also need to check the preceeding entry, if any, or if the closest match was above us
  if (UNLIKELY(closest_match != map_.begin() and closest_match->second.addr_start() > key)) {
    closest_match--;

    if (UNLIKELY(rng.overlaps(closest_match->second))){
      throw Memory_range_exception("Range '"+ std::string(rng.name())
                                   + "' overlaps with the range below key "
                                   + closest_match->second.to_string());
    }
  }

  auto new_entry = map_.emplace(key, std::move(rng));
  debug("* Range inserted (success: %i)", new_entry.second);

  return new_entry.first->second;
};

ptrdiff_t Memory_map::resize(uintptr_t key, ptrdiff_t size) {

  auto& range = map_.at(key);
  debug("Resize range 0x%x using %lib to use %lib", key, range.in_use(), size);

  if (range.in_use() >= size)
    throw Memory_range_exception("Can't resize. Range " + std::string(range.name())
                                 + " uses " + std::to_string(range.in_use())
                                 + "b, more than the requested " + std::to_string(size) + "b");

  auto collision = in_range(range.addr_start() + size);
  if (collision and collision != range.addr_start())
    throw Memory_range_exception("Can't resize. Range would collide with "
                                 + at(collision).to_string());

  return range.resize(size);

}
