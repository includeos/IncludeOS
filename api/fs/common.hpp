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

  typedef std::shared_ptr<uint8_t> buffer_t;

  struct error_t
  {
    enum token_t {
      NO_ERR = 0,
      E_IO, // general I/O error
      E_MNT,
      
      E_NOENT,
      E_NOTDIR,
    };
    
    error_t(token_t tk, const std::string& rsn)
      : token_(tk), reason_(rsn) {}
    
    // error code to string
    std::string to_string() const;
    // show explanation for error
    std::string reason() const noexcept {
      return reason_;
    }
    
    // returns true when it's an error
    operator bool () const noexcept {
      return token_ != NO_ERR;
    }
    
  private:
    token_t     token_;
    std::string reason_;
  };

  /** @var no_error: Always returns boolean false when used in expressions */
  extern error_t no_error;

} //< namespace fs

#endif //< FS_ERROR_HPP
