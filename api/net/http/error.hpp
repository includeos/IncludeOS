// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016 Oslo and Akershus University College of Applied Sciences
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
#ifndef HTTP_ERROR_HPP
#define HTTP_ERROR_HPP

#include <string>

namespace http {

  class Error {
  public:
    enum Code {
      NONE,
      RESOLVE_HOST,
      NO_REPLY,
      INVALID,
      TIMEOUT,
      CLOSING
    };

    Error(Code code = NONE)
      : code_{code}
    {}

    operator bool() const
    { return code_ != NONE; }

    Error code() const
    { return code_; }

    bool timeout() const
    { return code_ == TIMEOUT; }

    std::string to_string() const
    {
      switch(code_)
      {
        case NONE:
          return "No error";
        case RESOLVE_HOST:
          return "Unable to resolve host";
        case NO_REPLY:
          return "No reply";
        case TIMEOUT:
          return "Request timed out";
        case CLOSING:
          return "Connection closing";
        default:
          return "General error";
      } // < switch code_
    }

  private:
    Code code_;

  }; // < class Error

} // < namespace http

#endif // < HTTP_ERROR_HPP
