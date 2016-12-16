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

#include <mana/request.hpp>
#include <net/http/status_code_constants.hpp>

using namespace mana;

Request::OnRecv Request::on_recv_ = [](size_t) {};

Request::Request(buffer_t buf, size_t n)
  : Parent(std::string{(char*)buf.get(), n})
{

}

void Request::complete() {
  assert(is_complete());
  on_recv_(total_length());
}

size_t Request::content_length() const {
  using namespace http::header;
  if(!header().has_field(Content_Length))
    return 0;
  try {
    return std::stoull(header().value(Content_Length).to_string());
  }
  catch(...) {
    return 0;
  }
}

void Request::validate() const {
  using namespace http;
  switch(method())
  {
    case PUT:
    case POST: {
      if(content_length() == 0)
        throw Request_error{http::Length_Required,"Length required in POST/PUT"};
      return;
    }
    default: {
      if(content_length() > 0)
        throw Request_error{http::Bad_Request,"Content_Length is not allowed in GET"};
    }
  }
}

Request::~Request() {
  #ifdef VERBOSE_WEBSERVER
  //printf("<Request> Deleted\n");
  #endif
}
