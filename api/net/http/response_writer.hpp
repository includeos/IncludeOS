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

#pragma once
#ifndef HTTP_RESPONSE_WRITER_HPP
#define HTTP_RESPONSE_WRITER_HPP

// http
#include "response.hpp"
#include "connection.hpp"

#include <stdexcept>

namespace http {

  class Response_writer_error : public std::runtime_error {
    using base = std::runtime_error;
  public:
    using base::base;
  };

  class Response_writer;
  using Response_writer_ptr = std::unique_ptr<Response_writer>;

  /**
   * @brief      Helper for writing a HTTP Response on a connection.
   */
  class Response_writer {
  public:
    using buffer_t  = net::tcp::buffer_t;

  public:
    Response_writer(Response_ptr res, Connection&);

    auto& header()
    { return response_->header(); }

    const auto& header() const
    { return response_->header(); }

    /**
     * @brief      Write the response payload to the underlying connection.
     *             Writes header if not already written by write_header.
     *
     * @throws     Response_writer_error if somethings goes wrong/not allowed
     *
     * @param[in]  data  The data
     */
    void write(std::string data);

    /**
     * @brief      Same as write(std::string data) except data does not get copied into TCP buffer.
     *
     * @param[in]  chunk  a chunk of shared data
     */
    void write(net::tcp::buffer_t buffer);

    /**
     * @brief      Writes the status line + header to the underlying connection
     *
     * @throws     Response_writer_error when trying to do it more than once
     *
     * @param[in]  code  The code
     */
    void write_header(status_t code);

    /**
     * @brief      Writes the full response or just the body dependent if headers are sent or not.
     */
    void write();

    /**
     * @brief      Sets the response
     *
     * @param[in]  res   The response
     */
    void set_response(Response_ptr res)
    { Expects(not header_sent_); response_ = std::move(res); }

    Response& response()
    { return *response_; }

    const Response& response() const
    { return *response_; }

    Response_ptr& response_ptr()
    { return response_; }

    Connection& connection()
    { return connection_; }

    void end();

    ~Response_writer();

  private:
    Response_ptr  response_;
    Connection&   connection_;
    bool          header_sent_{false};

    /**
     * @brief      Preprocessing of a write
     *
     * @throws     Response_writer_error if something goes wrong/not allowed
     *
     * @param[in]  len   The length of the data to be written
     */
    void pre_write(size_t len);

  }; // < class Response_writer

} // < namespace http

#endif // < HTTP_RESPONSE_WRITER_HPP
