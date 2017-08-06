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

#ifndef HTTP_TIME_HPP
#define HTTP_TIME_HPP

#include <ctime>
#include <iomanip>
#include <sstream>

#include "../../util/detail/string_view"

namespace http {
namespace time {

///
/// Get the time in {Internet Standard Format} from
/// a {time_t} object
///
/// @param time_ The {time_t} object to get the time from
///
/// @return The time in {Internet Standard Format} as a std::string
///
/// @note Returns an empty string if an error occurred
///
std::string from_time_t(const std::time_t time_);

///
/// Get a {time_t} object from a {std::string} representing
/// timestamps specified in RFC 2616 §3.3
///
/// @param time_ The {std::string} representing the timestamp
///
/// @return A {time_t} object from {time_}
///
/// @note Returns a default initialized {time_t} object if an error occurred
///
std::time_t to_time_t(util::csview time_);

///
/// Get the current time in {Internet Standard Format}
///
/// @return The current time as a std::string
///
/// @note Returns an empty string if an error occurred
///
std::string now();

} //< namespace time
} //< namespace http

#endif //< HTTP_TIME_HPP
