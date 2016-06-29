#include <array>

#include <fs/filesystem.hpp>

namespace fs
{
  error_t no_error { error_t::NO_ERR, "" };

  const std::string& error_t::token() const noexcept {
    const static std::array<std::string, 6> tok_str
    {{
      "No error",
      "General I/O error",
      "Mounting filesystem failed",
      "No such entry",
      "Not a directory",
      "Not a file"
    }};

    return tok_str[token_];
  }

}
