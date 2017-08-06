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

#ifndef HTTP_STATUS_CODES_HPP
#define HTTP_STATUS_CODES_HPP

#include "status_code_constants.hpp"
#include "../../util/detail/string_view"

namespace http {

util::sview code_description(const status_t status_code) noexcept;

template<typename = void>
inline bool is_informational(const status_t status_code) noexcept {
  return (status_code >= Continue) and (status_code <= Processing);
}

template<typename = void>
inline bool is_success(const status_t status_code) noexcept {
  return (status_code >= OK) and (status_code <= IM_Used);
}

template<typename = void>
inline bool is_redirection(const status_t status_code) noexcept {
  return (status_code >= Multiple_Choices) and (status_code <= Permanent_Redirect);
}

template<typename = void>
inline bool is_client_error(const status_t status_code) noexcept {
  return (status_code >= Bad_Request) and (status_code <= Request_Header_Fields_Too_Large);
}

template<typename = void>
inline bool is_server_error(const status_t status_code) noexcept {
  return (status_code >= Internal_Server_Error) and (status_code <= Network_Authentication_Required);
}

} //< namespace http

#endif //< HTTP_STATUS_CODES_HPP
