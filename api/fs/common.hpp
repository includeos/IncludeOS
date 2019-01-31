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
#include <pmr>
#include <vector>
#include "path.hpp"
#include <common>
#include <delegate>
#include <hw/block_device.hpp>

namespace fs {

  struct Dirent;
  struct File_system;

  /**
   * @brief Shared vector used as a buffer within the filesystem subsystem
   */
  using buffer_t = os::mem::buf_ptr;

  /** Construct a shared vector **/
  template <typename... Args>
  buffer_t construct_buffer(Args&&... args) {
    return std::make_shared<os::mem::buffer> (std::forward<Args> (args)...);
  }

  /** Container types **/
  using dirvector  = std::vector<Dirent>;
  using Dirvec_ptr = std::shared_ptr<dirvector>;

  /** Pointer types **/
  using Path_ptr = std::shared_ptr<Path>;

  /** Entity types for dirents **/
  enum Enttype {
    FILE,
    DIR,
    /** FAT puts disk labels in the root directory, hence: */
    VOLUME_ID,
    SYM_LINK,

    INVALID_ENTITY
  };


  /** Error type **/
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
    Buffer(const error_t& e, buffer_t b)
      : err_    {e}, buffer_ {b} {}

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

    /// retrieve error status
    error_t error() const noexcept
    { return err_; }

    /**
     * Returns the underlying buffer
    **/
    auto& get() noexcept { return this->buffer_; }

    /**
     * @brief Get the starting address of the underlying data buffer
     *
     * @return The starting address of the underlying data buffer
     */
    const uint8_t* data() const noexcept
    { return buffer_->data(); }
    uint8_t* data() noexcept
    { return buffer_->data(); }

    /**
     * @brief Get the size/length of the buffer
     *
     * @return The size/length of the buffer
     */
    size_t   size() const noexcept
    {
      if (UNLIKELY(buffer_ == nullptr)) return 0;
      return buffer_->size();
    }

    /**
     * @brief Get a {std::string} representation of this type
     *
     * @return A {std::string} representation of this type
     */
    std::string to_string() const noexcept
    { return std::string{(const char*) data(), size()}; }

  private:
    error_t  err_;
    buffer_t buffer_;
  }; //< struct Buffer

  /** @var no_error: Always returns boolean false when used in expressions */
  extern error_t no_error;

  /** Async function types **/
  using on_init_func  = delegate<void(error_t, File_system&)>;
  using on_ls_func    = delegate<void(error_t, Dirvec_ptr)>;
  using on_read_func  = delegate<void(error_t, buffer_t)>;
  using on_stat_func  = delegate<void(error_t, Dirent)>;

  struct List
  {
    error_t    error;
    Dirvec_ptr entries;
    auto begin() { return entries->begin(); }
    auto end()   { return entries->end(); }
    auto cbegin() const { return entries->cbegin(); }
    auto cend()   const { return entries->cend(); }
  };

} //< fs

#endif //< FS_COMMON_HPP
