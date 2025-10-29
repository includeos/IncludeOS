#ifndef UTIL_PRETTY_HPP
#define UTIL_PRETTY_HPP

#include <format>
#include <string>

namespace util::format {
  inline std::string to_human_size(std::size_t bytes) {
    constexpr std::string_view size_suffixes[] = {
      "B", "KiB", "MiB", "GiB", "TiB", "PiB"
    };
    double value = static_cast<double>(bytes);
    std::size_t exponent = 0;

    while (value >= 1024.0 && exponent + 1 < std::size(size_suffixes)) {
      value /= 1024.0;
      exponent++;
    }

    if (exponent == 0)
      return std::format("{} {}", static_cast<std::uint64_t>(value), size_suffixes[exponent]);
    else
      return std::format("{:.2f} {}", value, size_suffixes[exponent]);
  }

  inline std::string with_thousands_sep(uint64_t value, char sep = '_', char every = 3) {
      std::string s = std::to_string(value);
      for (int i = s.size() - every; i > 0; i -= every)
          s.insert(i, 1, sep);
      return s;
  }
}  // namespace util

#endif // UTIL_PRETTY_HPP

