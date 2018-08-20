// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

#pragma once
#ifndef UTIL_URI_HPP
#define UTIL_URI_HPP

#include <vector>
#include <string>
#include <unordered_map>

#include "detail/string_view"

namespace uri {

#ifdef URI_THROW_ON_ERROR
#include <stdexcept>

///
/// This type is used to represent an error that occurred
/// from within the operations of class URI
///
struct URI_error : public std::runtime_error {
  using runtime_error::runtime_error;
}; //< struct URI_error

#endif //< URI_THROW_ON_ERROR

///
/// Representation of RFC 3986 URI's.
/// Ref. https://tools.ietf.org/html/rfc3986
///
class URI {
  public:
  ///
  /// Default constructor
  ///
  explicit URI() = default;

  ///
  /// Copy constructor
  ///
  URI(const URI&);

  ///
  /// Move constructor
  ///
  URI(URI&&) noexcept;

  ///
  /// Default destructor
  ///
  ~URI() = default;

  ///
  /// Default copy assignment operator
  ///
  URI& operator=(const URI&);

  ///
  /// Default move assignment operator
  ///
  URI& operator=(URI&&) noexcept;

  ///
  /// Construct using a C-String representing a uri
  ///
  /// @param uri
  ///   A C-String representing a uri
  ///
  /// @param parse
  ///   Whether to perform parsing on the the data specified in {uri}
  ///
  URI(const char* uri, const bool parse = true);

  ///
  /// Construct using a C-String representing a uri
  ///
  /// @param uri
  ///   A view of a string representing a uri
  ///
  /// @param count
  ///   The number of char's from {uri} representing a uri
  ///
  /// @param parse
  ///   Whether to perform parsing on the the data specified in {uri}
  ///
  URI(const char* uri, const size_t count, const bool parse = true);

  ///
  /// Construct using a std::string representing a uri
  ///
  /// @param uri
  ///   A std::string representing a uri
  ///
  /// @param parse
  ///   Whether to perform parsing on the the data specified in {uri}
  ///
  URI(const std::string& uri, const bool parse = true);

  ///
  /// Construct using a view of a string representing a uri
  ///
  /// @param uri
  ///   A view of a string representing a uri
  ///
  /// @param parse
  ///   Whether to perform parsing on the the data specified in {uri}
  ///
  URI(util::csview uri, const bool parse = true);

  ///////////////////////////////////////////////
  //----------RFC-specified URI parts----------//
  ///////////////////////////////////////////////

  ///
  /// Get scheme.
  ///
  /// @example 'http', 'file', 'ftp' etc.
  ///
  /// @return The scheme
  ///
  util::sview scheme() const noexcept;

  /**
   * @brief      Check whether the scheme is secure (like https or wss) or not.
   *
   * @return     true if secure, false otherwise
   */
  bool scheme_is_secure() const noexcept;

  ///
  /// Get userinfo.
  ///
  /// @example 'username:password'...
  ///
  /// @return The user's information
  ///
  util::sview userinfo() const noexcept;

  ///
  /// Get host.
  ///
  /// @example 'includeos.org', '10.0.0.42' etc.
  ///
  /// @return The host's information
  ///
  util::sview host() const noexcept;

  ///
  /// Check if host portion is an IPv4 address.
  ///
  /// @return True, maybe.
  ///
  bool host_is_ip4() const noexcept;

  ///
  /// Check if host portion is an IPv6 address.
  ///
  /// @return True, maybe.
  ///
  bool host_is_ip6() const noexcept;

  ///
  /// Get host and port information
  ///
  /// Format <host>:<port>
  ///
  /// @return host and port information
  ///
  std::string host_and_port() const;

  ///
  /// Get numeric port number.
  ///
  /// @warning The RFC don't specify dimension. This method will bind
  /// invalid port numbers to 65535
  ///
  /// @return The numeric port number as a 16-bit number
  ///
  uint16_t port() const noexcept;

  ///
  /// Get the path.
  ///
  /// @example /pictures/logo.png
  ///
  /// @return The path information
  ///
  util::sview path() const noexcept;

  ///
  /// Get the complete unparsed query string.
  ///
  /// @return The complete unparsed query string
  ///
  util::sview query() const noexcept;

  ///
  /// Get the fragment part.
  ///
  /// @example "...#anchor1"
  ///
  /// @return the fragment part
  ///
  util::sview fragment() const noexcept;

  ///
  /// Get the URI-decoded value of a query-string key.
  ///
  /// @param key
  /// The key to find the associated value
  ///
  /// @return The key's associated value
  ///
  /// @example For the query: "?name=Bjarne%20Stroustrup",
  /// query("name") returns "Bjarne Stroustrup"
  ///
  util::sview query(util::csview key);

  ///
  /// Check to see if an object of this type is valid
  ///
  /// @return true if valid, false otherwise
  ///
  bool is_valid() const noexcept;

  ///
  /// Convert an object of this type to a boolean value
  ///
  /// @see is_valid
  ///
  /// @return true if valid, false otherwise
  ///
  operator bool() const noexcept;

  ///
  /// Get a string representation of this class
  ///
  /// @return A string representation of this class
  ///
  std::string to_string() const;

  ///
  /// Operator to transform this class into string form
  ///
  operator std::string () const;

  ///
  /// Stream a chunk of new data into the uri for parsing
  ///
  /// @param chunk A new set of data to append to uri for parsing
  ///
  /// @return The object that invoked this method
  ///
  URI& operator << (const std::string& chunk);

  ///
  /// Parse the information supplied to the URI object
  ///
  /// @return The object that invoked this method
  ///
  URI& parse();

  ///
  /// Reset the object as if default constructed
  ///
  /// @return The object that invoked this method
  ///
  URI& reset();
private:
  ///
  /// A copy of the data representing a uri
  ///
  std::vector<char> uri_str_;

  uint16_t port_ {0xFFFF};

  util::sview scheme_;
  util::sview userinfo_;
  util::sview host_;
  util::sview path_;
  util::sview query_;
  util::sview fragment_;

  std::unordered_map<util::sview, util::sview> query_map_;

  ///
  /// Load queries into the map
  ///
  void load_queries();
}; //< class URI

///
/// Less-than operator to compare two {URI} objects
///
/// @param lhs
/// {URI} object to compare
///
/// @param rhs
/// {URI} object to compare
///
/// @return true if lhs is less-than rhs, false otherwise
///
bool operator < (const URI& lhs, const URI& rhs) noexcept;

///
/// Operator to compare two {URI} objects for equality
///
/// @param lhs
/// {URI} object to compare
///
/// @param rhs
/// {URI} object to compare
///
/// @return true if equal, false otherwise
///
/// @todo IPv6 authority comparison
///
bool operator == (const URI& lhs, const URI& rhs) noexcept;

///
/// Operator to stream the contents of a {URI} to the specified
/// output stream device
///
/// @param output_device
/// The output stream device
///
/// @param uri
/// The {URI} to send to the output stream
///
/// @return A reference to the specified output stream device
///
std::ostream& operator<< (std::ostream& output_device, const URI& uri);

} //< namespace uri

#endif //< UTIL_URI_HPP
