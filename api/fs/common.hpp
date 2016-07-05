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

  /**
   * @brief Type used as a building block to represent buffers
   * within the filesystem subsystem
   */
  using buffer_t = std::shared_ptr<uint8_t>;

  struct error_t
  {
    enum token_t {
      NO_ERR = 0,
      E_IO, // general I/O error
      E_MNT,

      E_NOENT,
      E_NOTDIR,
      E_NOTFILE
    }; //< enum token_t

    /**
     * @brief Constructor
     *
     * @param tk:  An error token
     * @param rsn: The reason for the error
     */
    error_t(const token_t tk, const std::string& rsn)
      : token_{tk}
      , reason_{rsn}
    {}

    /**
     * @brief Get a human-readable description of the token
     *
     * @return Description of the token as a {std::string}
     */
    const std::string& token() const noexcept;

    /**
     * @brief Get an explanation for error
     */
    const std::string& reason() const noexcept
    { return reason_; }

    /**
     * @brief Get a {std::string} representation of this type
     *
     * Format "description: reason"
     *
     * @return {std::string} representation of this type
     */
    std::string to_string() const
    { return token() + ": " + reason(); }

    /**
     * @brief Check if the object of this type represents
     * an error
     *
     * @return true if its an error, false otherwise
     */
    operator bool () const noexcept
    { return token_ not_eq NO_ERR; }

  private:
    token_t     token_;
    std::string reason_;
  }; //< struct error_t

  /**
   * @brief Type used for buffers within the filesystem
   * subsystem
   */
  struct Buffer
  {
    Buffer(const error_t& e, buffer_t b, const uint64_t l)
      : err_    {e}
      , buffer_ {b}
      , len_    {l}
    {}

    /**
     * @brief Check if an object of this type is in a valid
     * state
     *
     * @return true if valid, false otherwise
     */
    bool is_valid() const noexcept
    { return (buffer_ not_eq nullptr) and (not err_); }

    /**
     * @brief Coerce an object of this type to a bool
     */
    operator bool () const noexcept
    { return is_valid(); }

    /**
     * @brief Get the starting address of the underlying data buffer
     *
     * @return The starting address of the underlying data buffer
     */
    uint8_t* data() noexcept
    { return buffer_.get(); }

    /**
     * @brief Get the size/length of the buffer
     *
     * @return The size/length of the buffer
     */
    size_t   size() const noexcept
    { return len_; }

    /**
     * @brief Get a {std::string} representation of this type
     *
     * @return A {std::string} representation of this type
     */
    std::string to_string() const noexcept
    { return std::string{reinterpret_cast<char*>(buffer_.get()), size()}; }

  private:
    error_t  err_;
    buffer_t buffer_;
    uint64_t len_;
  }; //< struct Buffer

  /** @var no_error: Always returns boolean false when used in expressions */
  extern error_t no_error;

} //< namespace fs

#endif //< FS_ERROR_HPP
