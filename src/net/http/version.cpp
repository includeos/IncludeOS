
#include <cstdlib>
#include <net/http/version.hpp>

namespace http {

///////////////////////////////////////////////////////////////////////////////
Version::Version(const unsigned major, const unsigned minor) noexcept:
  major_{major},
  minor_{minor}
{}

///////////////////////////////////////////////////////////////////////////////
unsigned Version::major() const noexcept {
  return major_;
}

///////////////////////////////////////////////////////////////////////////////
void Version::set_major(const unsigned major) noexcept {
  major_ = major;
}

///////////////////////////////////////////////////////////////////////////////
unsigned Version::minor() const noexcept {
  return minor_;
}

///////////////////////////////////////////////////////////////////////////////
void Version::set_minor(const unsigned minor) noexcept {
  minor_ = minor;
}

///////////////////////////////////////////////////////////////////////////////
std::string Version::to_string() const {
  char http_ver[12];
  snprintf(http_ver, sizeof(http_ver), "%s%u.%u", "HTTP/", major_, minor_);
  return http_ver;
}

///////////////////////////////////////////////////////////////////////////////
Version::operator std::string() const {
  return to_string();
}

///////////////////////////////////////////////////////////////////////////////
bool operator==(const Version& lhs, const Version& rhs) noexcept {
  return lhs.major() == rhs.major()
         and
         lhs.minor() == rhs.minor();
}

///////////////////////////////////////////////////////////////////////////////
bool operator!=(const Version& lhs, const Version& rhs) noexcept {
  return not (lhs == rhs);
}

///////////////////////////////////////////////////////////////////////////////
bool operator<(const Version& lhs, const Version& rhs) noexcept {
  return (lhs.major() == rhs.major()) ? (lhs.minor() < rhs.minor()) : (lhs.major() < rhs.major());
}

///////////////////////////////////////////////////////////////////////////////
bool operator>(const Version& lhs, const Version& rhs) noexcept {
  return (lhs.major() == rhs.major()) ? (lhs.minor() > rhs.minor()) : (lhs.major() > rhs.major());
}

///////////////////////////////////////////////////////////////////////////////
bool operator<=(const Version& lhs, const Version& rhs) noexcept {
  return (lhs < rhs) or (lhs == rhs);
}

///////////////////////////////////////////////////////////////////////////////
bool operator>=(const Version& lhs, const Version& rhs) noexcept {
  return (lhs > rhs) or (lhs == rhs);
}

} //< namespace http
