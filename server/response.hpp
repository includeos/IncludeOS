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

#ifndef SERVER_RESPONSE_HPP
#define SERVER_RESPONSE_HPP

#include "http/inc/response.hpp"
#include "http/inc/mime_types.hpp"
#include "cookie.hpp"
#include <fs/filesystem.hpp>
#include <net/tcp/connection.hpp>
#include <utility/async.hpp>

#include <string>
#include <vector>
#include <time.h>
#include <chrono>

using namespace cookie;

struct File {

  File(fs::Disk_ptr dptr, const fs::Dirent& ent)
    : disk(dptr)
  {
    assert(ent.is_file());
    entry = ent;
  }

  fs::Dirent entry;
  fs::Disk_ptr disk;
};

namespace server {

class Response;
using Response_ptr = std::shared_ptr<Response>;

class Response : public http::Response {
private:
  using Code = http::status_t;
  using Connection_ptr = net::tcp::Connection_ptr;
  using OnSent = std::function<void(size_t)>;

public:

  struct Error {
    Code code;
    std::string type;
    std::string message;

    Error() : code{http::Bad_Request} {}

    Error(std::string&& type, std::string&& msg)
      : code{http::Bad_Request}, type{type}, message{msg}
    {}

    Error(const Code code, std::string&& type, std::string&& msg)
      : code{code}, type{type}, message{msg}
    {}

    // TODO: NotLikeThis
    std::string json() const {
      return "{ \"type\" : \"" + type + "\", \"message\" : \"" + message + "\" }";
    }
  };

  Response(Connection_ptr conn);

  /*
    Send only status code
  */
  void send_code(const Code);

  /*
    Send the Response
  */
  void send(bool close = false) const;

  /*
    Send a file
  */
  void send_file(const File&);

  void send_json(const std::string&);

  /* Cookie-support start */

  void cookie(const Cookie& c);

  void cookie(const std::string& name, const std::string& value);

  void cookie(const std::string& name, const std::string& value, const std::vector<std::string>& options);

  void update_cookie(const std::string& name, const std::string& old_path, const std::string& old_domain,
    const std::string& new_value);

  void update_cookie(const std::string& name, const std::string& old_path, const std::string& old_domain,
    const std::string& new_value, const std::vector<std::string>& new_options);

  void clear_cookie(const std::string& name, const std::string& path, const std::string& domain);

  /* Cookie-support end */

  /**
   * @brief Send an error response
   * @details Sends an error response together with the given status code.
   *
   * @param e Response::Error
   */
  void error(Error&&);

  /*
    "End" the response
  */
  void end() const;

  static void on_sent(OnSent cb)
  { on_sent_ = cb; }

  ~Response();

private:
  Connection_ptr conn_;

  static OnSent on_sent_;

  bool keep_alive = true;

  void write_to_conn(bool close_on_written = false) const;

}; // server::Response


} // < server

#endif
