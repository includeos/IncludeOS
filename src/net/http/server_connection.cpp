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

#include <net/http/server_connection.hpp>
#include <net/http/server.hpp>

namespace http {

 Server_connection::Server_connection(Server& server, Stream_ptr stream, size_t idx, const size_t bufsize)
    : Connection(std::move(stream)),
      server_(server),
      req_(nullptr),
      idx_(idx),
      idle_since_{0}
  {
    stream_->on_read(bufsize, {this, &Server_connection::recv_request});
    // setup close event
    stream_->on_close({this, &Server_connection::close});
  }

  void Server_connection::send(Response_ptr res)
  {
    stream_->write(res->to_string());
  }

  void Server_connection::recv_request(buffer_t buf)
  {
    if (buf->empty()) {
      //end_response({Error::NO_REPLY});
      return;
    }

    const std::string data{(char*) buf->data(), buf->size()};

    // create response if not exist
    if(req_ == nullptr)
    {
      try {
        req_ = make_request(data); // this also parses
        update_idle();
      }
      catch(...)
      {
        // Invalid request, just return (?)
        //end_request(http::Bad_Request);
        return;
      }
    }
    // if there already is a request
    else
    {
      // note: need to validate that the method is allowed, etc.
      // add chunks of body data
      req_->add_chunk(std::move(data));
      update_idle();
    }

    const auto& header = req_->header();

    if(not header.is_empty() && header.has_field(header::Content_Length))
    {
      try
      {
        const unsigned conlen = std::stoul(std::string(header.value(header::Content_Length)));
        // risk buffering forever if no timeout
        if(conlen == req_->body().size())
        {
          end_request();
        }
      }
      catch(...)
      {
        end_request(http::Bad_Request);
      }
    }
    else
    {
      end_request();
    }
  }

  void Server_connection::end_request(const status_t code)
  {
    server_.receive(std::move(req_), code, *this);
  }

  void Server_connection::close()
  {
    server_.close(*this);
  }

}
