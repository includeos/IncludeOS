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

  Response_writer::Response_writer(Response_ptr res, Connection& conn)
    : response_(std::move(res)),
      connection_(conn)
  {
    Ensures(not connection_.released());
  }

  void Response_writer::write(std::string data)
  {
    pre_write(data.size());

    connection_.stream()->write(std::move(data));
  }

  void Response_writer::write(net::tcp::buffer_t buffer)
  {
    pre_write(buffer->size());

    connection_.stream()->write(std::move(buffer));
  }

  void Response_writer::pre_write(size_t len)
  {
    // send headers if not already sent
    if(not header_sent_)
    {
      // lets help out with setting the content-length if none is set
      if(not header().has_field(header::Content_Length))
      {
        response_->set_content_length(len);
      }
      // content-length is already set
      else
      {
        const auto cl = response_->content_length();
        // don't allow writing more than content-length in header allows
        if(cl < len)
          throw Response_writer_error{"Trying to write more than Content-Length allows: " + std::to_string(cl)};
      }
      // write headers
      write_header(http::OK);
    }
    else
    {
      const auto cl = response_->content_length();
      // don't allow writing more than content-length in header allows
      if(cl < len)
        throw Response_writer_error{"Trying to write more than Content-Length allows: " + std::to_string(cl)};
    }
  }

  void Response_writer::write_header(status_t code)
  {
    if(LIKELY(not header_sent_))
    {
      response_->set_status_code(code);

      std::ostringstream header;
      header << response_->status_line() << "\r\n" << response_->header();

      connection_.stream()->write(header.str());

      // disable keep alive if "Connection: close" is present
      if(response_->header().value(http::header::Connection) == "close")
        connection_.keep_alive(false);
    }
    else
      throw Response_writer_error{"Headers already sent."};
  }

  void Response_writer::write()
  {
    if(!response_->body().empty())
      write(std::string(response_->body()));
    else
      write_header(response_->status_code());
  }

  void Response_writer::end()
  {
    connection_.end();
  }

  Response_writer::~Response_writer()
  {
    end();
  }

}
