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

#ifndef MANA_RESPONSE_HPP
#define MANA_RESPONSE_HPP

#include <net/http/response.hpp>
#include <fs/filesystem.hpp>
#include <net/tcp/connection.hpp>
#include <util/async.hpp>

#include <string>
#include <vector>
#include <time.h>
#include <chrono>

struct File {

  File(fs::Disk_ptr dptr, const fs::Dirent& ent)
    : disk(dptr), entry(ent)
  {
    assert(entry.is_file());
  }

  fs::Disk_ptr disk;
  fs::Dirent entry;
};

namespace mana {

class Response;
using Response_ptr = std::shared_ptr<Response>;

class Response : public http::Response {
private:
  using Code = http::status_t;
  using Connection_ptr = net::tcp::Connection_ptr;
  using OnSent = delegate<void(size_t)>;

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
  void send_code(const Code, bool close = true);

  /*
    Send the Response
  */
  void send(bool close = false);

  /*
    Send a file
  */
  void send_file(const File&);

  void send_json(const std::string&);

  /** Cookies */

  void cookie(const std::string& cookie)
  { header().set_field(http::header::Set_Cookie, cookie); }

  template <typename Cookie>
  void cookie(const Cookie& c)
  { cookie(c.to_string()); }

  template <typename Cookie>
  inline void clear_cookie(const std::string& name)
  { clear_cookie<Cookie>(name, "", ""); }

  template <typename Cookie>
  inline void clear_cookie(const std::string& name, const std::string& path, const std::string& domain);

  template <typename Cookie>
  inline void update_cookie(const std::string& name, const std::string& new_value)
  { update_cookie<Cookie>(name, "", "", new_value); }

  template <typename Cookie>
  inline void update_cookie(const std::string& name, const std::string& new_value,
      const std::vector<std::string>& new_options)
  { update_cookie<Cookie>(name, "", "", new_value, new_options); }

  template <typename Cookie>
  inline void update_cookie(const std::string& name, const std::string& old_path, const std::string& old_domain,
    const std::string& new_value);

  template <typename Cookie>
  inline void update_cookie(const std::string& name, const std::string& old_path, const std::string& old_domain,
    const std::string& new_value, const std::vector<std::string>& new_options);


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

  void write_to_conn(bool close_on_written = false);

}; // < class Response

template <typename Cookie>
inline void Response::clear_cookie(const std::string& name, const std::string& path, const std::string& domain) {
  Cookie c{name, ""};
  c.set_path(path);
  c.set_domain(domain);
  c.set_expires("Sun, 06 Nov 1994 08:49:37 GMT"); // in the past

  cookie(c);
}

template <typename Cookie>
inline void Response::update_cookie(const std::string& name, const std::string& old_path, const std::string& old_domain,
  const std::string& new_value) {
  // 1. Clear old cookie:
  clear_cookie<Cookie>(name, old_path, old_domain);
  // 2. Set new cookie:
  Cookie new_cookie{name, new_value};
  cookie(new_cookie);
}

template <typename Cookie>
inline void Response::update_cookie(const std::string& name, const std::string& old_path, const std::string& old_domain,
  const std::string& new_value, const std::vector<std::string>& new_options) {
  // 1. Clear old cookie:
  clear_cookie<Cookie>(name, old_path, old_domain);
  // 2. Set new cookie:
  Cookie new_cookie{name, new_value, new_options};
  cookie(new_cookie);
}


} // < mana

#endif
