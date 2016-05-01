// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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
#ifndef FS_COMMON_HPP
#define FS_COMMON_HPP

#include <memory>
#include <string>

namespace fs {

  ///
  /// Underlying type for filesystem buffer representation
  ///
  using buffer_t = std::shared_ptr<uint8_t>;

  ///
  /// Data-structure used to represent a filesystem error
  ///
  struct error_t {

    ///
    /// Various types of errors that can occur during filesystem
    /// operations
    ///
    enum token_t {
      NO_ERR = 0,
      E_IO, // General I/O error
      E_MNT,

      E_NOENT,
      E_NOTDIR,
    };

    ///
    /// Constructor to create a filesystem error object
    ///
    /// @param tk
    ///   Type of error
    ///
    /// @param rsn
    ///   Reason for the error
    ///
    error_t(token_t tk, const std::string& rsn)
      : token_(tk)
      , reason_(rsn)
    {}

    ///
    /// Get a human-readable representation for the type of error
    ///
    /// @return
    ///   An STL-string describing the error
    ///
    std::string token() const noexcept;

    ///
    /// Get an explanation for the generated error
    ///
    /// @return
    ///   An STL-string explaining the reason why the error was generated
    ///
    std::string reason() const noexcept
    { return reason_; }

    ///
    /// Get a string representation of this data-structure
    ///
    /// @return
    ///   An STL-string in the following format:
    ///     "description: reason"
    ///
    std::string to_string() const noexcept
    { return token() + ": " + reason(); }

    ///
    /// Conversion of this data-structure to a boolean value
    ///
    /// @return
    ///   true when it's an error, false otherwise
    ///
    operator bool () const noexcept
    { return token_ != NO_ERR; }

  private:
    const token_t     token_;
    const std::string reason_;
  }; //< struct error_t

  ///
  /// Data-structure for filesystem buffer representation
  ///
  struct Buffer {

    ///
    /// Constructor to create a filesystem buffer object
    ///
    /// @param e
    ///   An error object to represent the state of the buffer
    ///
    /// @param b
    ///   The underlying buffer to encapsulate
    ///
    /// @param s
    ///   The size of the underlying buffer in bytes
    ///
    explicit Buffer(error_t e, buffer_t b, size_t s)
      : err_(e)
      , buffer_(b)
      , size_(s)
    {}

    ///
    /// Check to see if the buffer is in a valid state
    ///
    /// @return
    ///   true if this buffer is valid, false otherwise
    ///
    bool is_valid() const noexcept
    { return (buffer_ not_eq nullptr) and (err_ == false; }

    ///
    /// Conversion of this data-structure to a boolean value
    ///
    /// @return
    ///   true if this buffer is valid, false otherwise
    ///
    operator bool () const noexcept
    { return is_valid(); }

    ///
    /// Get the object that represents the state of the buffer
    ///
    /// @return
    ///   The object that represents the state of the buffer
    ///
    error_t err() const noexcept
    { return err_; }

    ///
    /// Get a raw handle (pointer) to the underlying buffer
    ///
    /// @return
    ///   A raw handle (pointer) to the underlying buffer
    ///
    uint8_t* data()
    { return buffer_.get(); }

    ///
    /// Get the size of the underlying buffer
    ///
    /// @return
    ///   The size of the underlying buffer in bytes
    ///
    size_t size() const noexcept
    { return size_; }

    ///
    /// Get a string representation of this data-structure
    ///
    /// @return
    ///   An STL-string that contains the information in the underlying buffer
    ///
    std::string to_string() const noexcept
    { return std::string{static_cast<char*>(buffer_.get()), size()}; }

  private:
    const error_t  err_;
    buffer_t       buffer_;
    const uint64_t size_;
  }; //< struct Buffer

  ///
  /// @var no_error
  ///   Always returns a boolean value of false when used in expressions
  ///
  extern const error_t no_error;

} //< namespace fs

#endif //< FS_ERROR_HPP
