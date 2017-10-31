// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef HTTP_VERSION_HPP
#define HTTP_VERSION_HPP

#include <ostream>

#ifdef __GNUC__
#undef major
#undef minor
#endif //< __GNUC__

namespace http {

///
/// This class represents the version of an http message
///
class Version {
public:
  ///
  /// Constructor
  ///
  /// @param major The major version number
  /// @param minor The minor version number
  ///
  explicit Version(const unsigned major = 1U, const unsigned minor = 1U) noexcept;

  ///
  /// Default destructor
  ///
  ~Version() noexcept =default;

  ///
  /// Default copy constructor
  ///
  Version(const Version&) noexcept =default;

  ///
  /// Default move constructor
  ///
  Version(Version&&) noexcept =default;

  ///
  /// Default assignment operator
  ///
  Version& operator=(const Version&) noexcept =default;

  ///
  /// Default move assignment operator
  ///
  Version& operator=(Version&&) noexcept =default;

  ///
  /// Get the major version number
  ///
  /// @return The major version number
  ///
  unsigned major() const noexcept;

  ///
  /// Set the major version number
  ///
  /// @param major The major version number
  ///
  void set_major(const unsigned major) noexcept;

  ///
  /// Get the minor version number
  ///
  /// @return The minor version number
  ///
  unsigned minor() const noexcept;

  ///
  /// Set the minor version number
  ///
  /// @param minor The minor version number
  ///
  void set_minor(const unsigned minor) noexcept;

  ///
  /// Get a string representation of this
  /// class
  ///
  /// @return A string representation
  ///
  std::string to_string() const;

  ///
  /// Operator to transform this class
  /// into string form
  ///
  operator std::string() const;
private:
  ///
  /// Class data members
  ///
  unsigned major_;
  unsigned minor_;
}; //< class Version

/**--v----------- Helper Functions -----------v--**/

///
/// Operator to check for equality
///
bool operator==(const Version& lhs, const Version& rhs) noexcept;

///
/// Operator to check for inequality
///
bool operator!=(const Version& lhs, const Version& rhs) noexcept;

///
/// Operator to check for less than relationship
///
bool operator<(const Version& lhs, const Version& rhs) noexcept;

///
/// Operator to check for greater than relationship
///
bool operator>(const Version& lhs, const Version& rhs) noexcept;

///
/// Operator to check for less than or equal to relationship
///
bool operator<=(const Version& lhs, const Version& rhs) noexcept;

///
/// Operator to check for greater than or equal to relationship
///
bool operator>=(const Version& lhs, const Version& rhs) noexcept;

/**--^----------- Helper Functions -----------^--**/

/**--v----------- Implementation Details -----------v--**/

///////////////////////////////////////////////////////////////////////////////
template<typename Char, typename Char_traits>
inline std::basic_ostream<Char, Char_traits>& operator<<(std::basic_ostream<Char, Char_traits>& output_device, const Version& version) {
  return output_device << version.to_string();
}

/**--^----------- Implementation Details -----------^--**/

} //< namespace http

#endif //< HTTP_VERSION_HPP
