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

#ifndef HTTP_MESSAGE_HPP
#define HTTP_MESSAGE_HPP

#include <sstream>

#include "header.hpp"
#include "time.hpp"

namespace http {

///
/// This is the base class of an HTTP message which contain
/// the headers and optional body
///
class Message {
private:
  ///
  /// Internal class type aliases
  ///
  using Message_body   = std::string;
public:
  ///
  /// Default constructor
  ///
  explicit Message() = default;

  ///
  /// Constructor to specify the limit of how many
  /// fields that can be added to the message
  ///
  /// @param limit Capacity of how many fields can
  /// be added to the message
  ///
  explicit Message(const std::size_t limit) noexcept;

  ///
  /// Default copy constructor
  ///
  Message(const Message&) = default;

  ///
  /// Default move constructor
  ///
  explicit Message(Message&&) noexcept = default;

  ///
  /// Default destructor
  ///
  virtual ~Message() noexcept = default;

  ///
  /// Default copy assignment operator
  ///
  Message& operator = (const Message&) = default;

  ///
  /// Default move assignment operator
  ///
  Message& operator = (Message&&) = default;

  ///
  /// Get a modifiable reference to the header object
  ///
  Header& header() noexcept;

  ///
  /// Get a read-only reference to the header object
  ///
  const Header& header() const noexcept;

  /**
   * @brief      Returns the Content-Length value in header as an integer
   *
   * @note       Return value 0 can mean its either unset or zero.
   *
   * @return     The content length as integer
   */
  inline size_t content_length() const;

  /**
   * @brief      Sets the Content-Length value in the header
   *
   * @param[in]  len   The length of the content
   *
   * @return     Outcome of whether the field got updated or not
   */
  inline bool set_content_length(size_t len);

  ///
  /// Add an entity to the message
  ///
  /// @param message_body The entity to be
  /// sent with the message
  ///
  /// @return The object that invoked this method
  ///
  Message& add_body(const Message_body& message_body);

  ///
  /// Append a chunk to the entity of the message
  ///
  /// @param chunk A chunk to append to the entity
  ///
  /// @return The object that invoked this method
  ///
  Message& add_chunk(const Message_body& chunk);

  ///
  /// Check if this message has an entity
  ///
  /// @return true if entity is present, false
  /// otherwise
  ///
  bool has_body() const noexcept;

  ///
  /// Get a view of the entity in this the message if
  /// present
  ///
  /// @return A view of the entity in this message
  ///
  util::sview body() const noexcept;

  ///
  /// Remove the entity from the message
  ///
  /// @return The object that invoked this method
  ///
  Message& clear_body() noexcept;

  ///
  /// Reset the message as if it was now default
  /// constructed
  ///
  /// @return The object that invoked this method
  ///
  virtual Message& reset() noexcept;

  ///
  /// Get a string representation of this
  /// class
  ///
  /// @return A string representation
  ///
  virtual std::string to_string() const;

  ///
  /// Operator to transform this class
  /// into string form
  ///
  operator std::string () const;

  ///
  /// Get a view of a buffer holding intermediate information
  ///
  /// @return A view of a buffer holding intermediate information
  ///
  util::sview private_field() const noexcept;

  ///
  /// Set the content of the buffer holding intermediate information
  ///
  void set_private_field(const char* base, const size_t length) noexcept;

  /**
   * @brief      Whether the headers are complete or not.
   *
   * @return     True if complete, False if not
   */
  inline bool headers_complete() const noexcept;

  /**
   * @brief      Sets the status whether the headers are complete or not
   *
   * @param[in]  complete  Indicates if complete
   */
  inline void set_headers_complete(const bool complete) noexcept;
private:
  ///
  /// Class data members
  ///
  Header       header_fields_;
  Message_body message_body_;
  util::sview  field_;
  bool         headers_complete_;
}; //< class Message

/**--v----------- Helper Functions -----------v--**/

///
/// Add a set of headers to a message
///
Message& operator << (Message& res, const Header_set& headers);

/**--^----------- Helper Functions -----------^--**/

/**--v-------- Inline Implementations --------v--**/
inline size_t Message::content_length() const {
  return header_fields_.content_length();
}

inline bool Message::set_content_length(const size_t len) {
  return header_fields_.set_content_length(len);
}

inline void Message::set_headers_complete(const bool complete) noexcept {
  headers_complete_ = complete;
}

inline bool Message::headers_complete() const noexcept {
  return headers_complete_;
}

/**--^-------- Inline Implementations --------^--**/

} //< namespace http

#endif //< HTTP_MESSAGE_HPP
