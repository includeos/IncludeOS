// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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

#ifndef NET_ERROR_HPP
#define NET_ERROR_HPP

#include <cstdint>
#include <string>

namespace net {

  /**
   *  General Error class for the OS
   *  ICMP_error f.ex. inherits from this class
   *
   *  Default: No error occurred
   */
  class Error {
  public:

    enum class Type : uint8_t {
      no_error,
      general_IO,
      ifdown,
      ICMP,
      timeout
      // Add more as needed
    };

    Error() = default;

    Error(Type t, const char* msg)
      : t_{t}, msg_{msg}
    {}

    virtual ~Error() = default;

    Type type()
    { return t_; }

    operator bool() const noexcept
    { return t_ != Type::no_error; }

    bool is_icmp() const noexcept
    { return t_ == Type::ICMP; }

    virtual const char* what() const noexcept
    { return msg_; }

    virtual std::string to_string() const
    { return std::string{msg_}; }

  private:
    Type t_{Type::no_error};
    const char* msg_{"No error"};

  };  // < class Error

} //< namespace net

#endif  //< NET_ERROR_HPP
