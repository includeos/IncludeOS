#include <fs/filesystem.hpp>

namespace fs
{
  error_t no_error { error_t::NO_ERR, "" };
  
  std::string error_t::to_string() const {
    switch (token_) {
    case NO_ERR:
      return "No error";
    case E_IO:
      return "General I/O error";
    case E_MNT:
      return "Mounting filesystem failed";
    
    case E_NOENT:
      return "No such entry";
    case E_NOTDIR:
      return "Not a directory";
    
    }
  }
  
}
