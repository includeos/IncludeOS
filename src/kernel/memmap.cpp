#include <kernel/memmap.hpp>


Fixed_memory_range& Memory_map::assign_range (Fixed_memory_range&& rng) {

  //INFO2("* Assgning range %s", rng.to_string().c_str());

  // Keys are address representations, not pointers
  auto key = reinterpret_cast<uintptr_t>(rng.cspan().data());

  if (UNLIKELY(map_.empty()))
    return map_.emplace(std::make_pair(key, rng)).first->second;

  // Make sure the range does not overlap with any existing ranges
  auto closest_match = map_.lower_bound(key);

  // If the new range starts above all other ranges, we still need to make sure the last element doesn't overlap
  if (closest_match == map_.end()) {
    closest_match --;
  }

  if (UNLIKELY(rng.overlaps(closest_match->second)) ){
    throw Memory_range_exception("Range '"+ std::string(rng.name())
                                 + "'overlaps with "
                                 + closest_match->second.to_string());
  }

  // We also need to check the preceeding entry, if any, or if the closest match was above us
  if (closest_match != map_.begin() and closest_match->second.addr_start() > key) {
    closest_match--;

    if (UNLIKELY(rng.overlaps(closest_match->second))){
      throw Memory_range_exception("Range '"+ std::string(rng.name())
                                   + "'overlaps with "
                                   + closest_match->second.to_string());
    }
  }

  auto new_entry = map_.emplace(std::make_pair(key, rng));
  debug("* Range inserted (success: %i)", new_entry.second);

  return new_entry.first->second;
};
