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

#ifndef HTTP_HEADER_HPP
#define HTTP_HEADER_HPP

#include <algorithm>
#include <cctype>
#include <cstring>
#include <ostream>
#include <type_traits>

#include "common.hpp"
#include "header_fields.hpp"

#include "../../util/detail/string_view"

namespace http {

///
/// This class is used to store header information
/// associated with an http message
///
/// By default it is limited to 25 fields
/// but the amount can be specified by using the
/// appropriate constructor
///
class Header {
private:
  ///
  /// Internal class type aliases
  ///
  using Const_iterator = Header_set::const_iterator;
public:
  ///
  /// Default constructor that limits the amount
  /// of fields that can be added to 25
  ///
  explicit Header();

  ///
  /// Constructor to specify the limit of how many
  /// fields that can be added
  ///
  /// @param limit Capacity of how many fields can
  /// be added
  ///
  explicit Header(const std::size_t limit);

  ///
  /// Default destructor
  ///
  ~Header() noexcept = default;

  ///
  /// Default copy constructor
  ///
  Header(const Header&) = default;

  ///
  /// Default move constructor
  ///
  Header(Header&&) noexcept = default;

  ///
  /// Default assignment operator
  ///
  Header& operator = (const Header&) = default;

  ///
  /// Default move assignemt operator
  ///
  Header& operator = (Header&&) noexcept = default;

  ///
  /// Add a new field to the current set
  ///
  /// @param field The field name
  /// @param value The field value
  ///
  /// @return true if the field was added, false
  /// otherwise
  ///
  bool add_field(std::string field, std::string value);

  ///
  /// Change the value of the specified field
  ///
  /// If the field is absent from the set it will
  /// be added with the associated value once its
  /// within capacity
  ///
  /// @param field The field name
  /// @param value The field value
  ///
  /// @return true if successful, false otherwise
  ///
  bool set_field(std::string field, std::string value);

  ///
  /// Check to see if the specified field is a
  /// member of the set
  ///
  /// @param field The field name
  ///
  /// @return true if the field is a member,
  /// false otherwise
  ///
  bool has_field(util::csview field) const noexcept;

  ///
  /// Get the value associated with a field
  ///
  /// @param field The field name
  ///
  /// @return The value associated with the specified field name, empty
  /// view otherwise
  ///
  ///
  util::sview value(util::csview field) const noexcept;

  ///
  /// Check to see if the set is empty
  ///
  /// @return true if there are no fields within
  /// the set, false otherwise
  ///
  bool is_empty() const noexcept;

  ///
  /// Check to see how many fields are currently
  /// in the set
  ///
  /// @return The amount of fields currently in
  /// the set
  ///
  std::size_t size() const noexcept;

  ///
  /// Remove all fields from the set with the
  /// specified name
  ///
  /// @param field The field name to remove
  ///
  void erase(util::csview field) noexcept;

  ///
  /// Remove all fields from the set leaving it
  /// empty
  ///
  void clear() noexcept;

  ///
  /// Returns the Content-Length value in header as an integer
  ///
  /// @note Return value 0 can mean its either unset or zero.
  ///
  /// @return The content length as integer
  ///
  size_t content_length() const noexcept;

  ///
  /// Sets the Content-Length value in the header
  ///
  /// @param len The length of the content
  ///
  /// @return Outcome of whether the field got updated or not
  ///
  bool set_content_length(const size_t len);

private:
  ///
  /// Class data members
  ///
  Header_set fields_;

  ///
  /// Find the location of a field within the set
  ///
  /// @param field The field name to locate
  ///
  /// @return Iterator to the location of the field,
  /// else location to the end of the sequence
  ///
  Const_iterator find(util::csview field) const noexcept;

  ///
  /// Operator to stream the contents of the set
  /// into the specified output device
  ///
  /// The format is as follows:
  /// field : value "\r\n"
  ///
  template<typename Char, typename Char_traits>
  friend std::basic_ostream<Char, Char_traits>& operator<<(std::basic_ostream<Char, Char_traits>& output_device, const Header& header);
}; //< class Header

template<typename Char, typename Char_traits>
std::basic_ostream<Char, Char_traits>& operator<<(std::basic_ostream<Char, Char_traits>& output_device, const Header& header) {
  if (not header.is_empty()) {
    for (const auto& field : header.fields_) {
      output_device << field.first  << ": "
                    << field.second << "\r\n";
    }
    //-----------------------------------
    output_device << "\r\n";
  }
  return output_device;
}

} //< namespace http

#endif //< HTTP_HEADER_HPP
