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

#pragma once
#ifndef URI_HPP
#define URI_HPP

#include <experimental/string_view>
#include <unordered_map>

namespace uri {

#ifdef URI_THROW_ON_ERROR
#include <stdexcept>

///
/// This class is used to represent an error that occurred
/// from within the operations of class URI
///
class URI_error : public std::runtime_error {
public:
  using runtime_error::runtime_error;
}; //< class URI_error

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
  /// Default copy constructor
  ///
  URI(const URI&) = default;

  ///
  /// Default move constructor
  ///
  URI(URI&&) = default;

  ///
  /// Default destructor
  ///
  ~URI() = default;

  ///
  /// Default assignment operator
  ///
  URI& operator=(const URI&) = default;

  ///
  /// Default move assignment operator
  ///
  URI& operator=(URI&&) = default;

  ///
  /// Construct using a view of a string representing a uri
  ///
  /// @param uri
  /// A view of a string representing a uri
  ///
  /// @param parse
  /// Whether to perform parsing on the the data specified in {uri}
  ///
  explicit URI(const std::experimental::string_view uri, const bool parse = true);

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
  std::experimental::string_view scheme() const noexcept;

  ///
  /// Get userinfo.
  ///
  /// @example 'username:password'...
  ///
  /// @return The user's information
  ///
  std::experimental::string_view userinfo() const noexcept;

  ///
  /// Get host.
  ///
  /// @example 'includeos.org', '10.0.0.42' etc.
  ///
  /// @return The host's information
  ///
  std::experimental::string_view host() const noexcept;

  ///
  /// Get the raw port number in decimal character representation.
  ///
  /// @return The raw port number in decimal character representation
  ///
  std::experimental::string_view port_str() const noexcept;

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
  std::experimental::string_view path() const noexcept;

  ///
  /// Get the complete unparsed query string.
  ///
  /// @return The complete unparsed query string
  ///
  std::experimental::string_view query() const noexcept;

  ///
  /// Get the fragment part.
  ///
  /// @example "...#anchor1"
  ///
  /// @return the fragment part
  ///
  std::experimental::string_view fragment() const noexcept;

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
  std::experimental::string_view query(const std::experimental::string_view key);

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
  std::experimental::string_view to_string() const noexcept;

  /**
   * @brief      Get the actual string the URI is built on
   *
   * @return     The string source of this class
   */
  const std::string& str() const noexcept;

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
private:
  ///
  /// A copy of the data representing a uri
  ///
  std::string uri_str_;

  mutable uint16_t port_ {0xFFFF};

  std::experimental::string_view scheme_;
  std::experimental::string_view userinfo_;
  std::experimental::string_view host_;
  std::experimental::string_view port_str_;
  std::experimental::string_view path_;
  std::experimental::string_view query_;
  std::experimental::string_view fragment_;

  std::unordered_map<std::experimental::string_view, std::experimental::string_view> query_map_;

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

#endif //< URI_HPP
