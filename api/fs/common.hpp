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
   * @brief Type used for buffers within the filesystem
   * subsystem
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
    };

    error_t(const token_t tk, const std::string& rsn) noexcept
      : token_{tk}
      , reason_{rsn}
    {}

    /**
     * @brief Get a human-readable description of the token
     *
     * @return Description of the token as a {std::string}
     */
    const std::string& token() const noexcept;

    // show explanation for error
    std::string reason() const noexcept {
      return reason_;
    }

    // returns "description": "reason"
    std::string to_string() const noexcept {
      return token() + ": " + reason();
    }

    // returns true when it's an error
    operator bool () const noexcept {
      return token_ != NO_ERR;
    }

  private:
    const token_t      token_;
    const std::string& reason_;
  }; //< struct error_t

  struct Buffer
  {
    Buffer(error_t e, buffer_t b, size_t l)
      : err(e), buffer(b), len(l) {}

    // returns true if this buffer is valid
    bool is_valid() const noexcept {
      return buffer != nullptr;
    }
    operator bool () const noexcept {
      return is_valid();
    }

    uint8_t* data() {
      return buffer.get();
    }
    size_t   size() const noexcept {
      return len;
    }

    // create a std::string from the stored buffer and return it
    std::string to_string() const noexcept {
      return std::string((char*) buffer.get(), size());
    }

    error_t  err;
    buffer_t buffer;
    uint64_t len;
  };

  /** @var no_error: Always returns boolean false when used in expressions */
  extern error_t no_error;

} //< namespace fs

#endif //< FS_ERROR_HPP
