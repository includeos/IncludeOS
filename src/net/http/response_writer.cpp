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

#include <net/http/response_writer.hpp>

#include <sstream>

namespace http {

  Response_writer::Response_writer(Response_ptr res, TCP_conn conn)
    : response_(std::move(res)),
      connection_(std::move(conn))
  {
  }

  void Response_writer::send_header(status_t code)
  {
    if(LIKELY(not header_sent_))
    {
      response_->set_status_code(code);

      std::ostringstream header;
      header << response_->status_line() << "\r\n" << response_->header();

      connection_->write(header.str());
    }
  }

  void Response_writer::send()
  {
    connection_->write(response_->to_string());
  }

  void Response_writer::send_body(std::string data)
  {
    if(not header_sent_)
      send_header();

    connection_->write(std::move(data));
  }

}
