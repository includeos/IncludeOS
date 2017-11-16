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

#ifndef HTTP_COMMON_HPP
#define HTTP_COMMON_HPP

#include <delegate>
#include <memory>
#include <uri>
#include <utility>
#include <vector>

#include "../../util/detail/string_view"

namespace http {

using URI = uri::URI;

using Header_set = std::vector<std::pair<std::string, std::string>>;

class Request;
using Request_ptr = std::unique_ptr<Request>;

class Response;
using Response_ptr = std::unique_ptr<Response>;

class Server;
using Server_ptr = std::unique_ptr<Server>;

} //< namespace http

#endif //< HTTP_COMMON_HPP
