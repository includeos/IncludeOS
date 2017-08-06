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

#ifndef HTTP_MIME_TYPES_HPP
#define HTTP_MIME_TYPES_HPP

#include "../../util/detail/string_view"

namespace http {

///
/// Get the mime type for the specified extension
///
/// @param extension The specified extension to get associated mime type
///
/// @return The associated mime type for the specified extension
///
util::sview ext_to_mime_type(util::csview extension) noexcept;

} //< namespace http

#endif //< HTTP_MIME_TYPES_HPP
