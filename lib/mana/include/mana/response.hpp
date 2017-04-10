// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

#include <fs/disk.hpp>
#include <fs/filesystem.hpp>
struct File {

  File(fs::Disk_ptr dptr, const fs::Dirent& ent)
    : disk(dptr), entry(ent)
  {
    assert(entry.is_file());
  }

  fs::Disk_ptr disk;
  fs::Dirent entry;
};

#include <net/http/response_writer.hpp>
#include <string>

namespace mana {

class Response;
using Response_ptr = std::shared_ptr<Response>;

class Response : public std::enable_shared_from_this<Response> {
private:
  using Code = http::status_t;

public:

  /**
   * @brief      An error to a HTTP Response
   */
  struct Error {
    Code code;
    std::string type;
    std::string message;

    /**
     * @brief     Constructs an error with code "Bad Request" and without type & msg
     */
    inline Error();

    /**
     * @brief      Constructs an error with code "Bad Request" with the given type & msg
     *
     * @param[in]  type  The type of the error as a string
     * @param[in]  msg   The error message as a string
     */
    inline Error(std::string&& type, std::string&& msg);

    /**
     * @brief      Constructs an error with a given code, type & msg
     *
     * @param[in]  code  The error code
     * @param[in]  type  The type of the error as a string
     * @param[in]  msg   The error message as a string
     */
    inline Error(const Code code, std::string&& type, std::string&& msg);

    /**
     * @brief      Represent the error's type and message as a json object
     *
     * @return     A json string in the form of { "type": <type>, "message": <message> }
     */
    inline std::string json() const;
  };

  /**
   * @brief      Construct a Response with a given HTTP Response writer
   *
   * @param[in]  reswriter  The HTTP response writer
   */
  explicit Response(http::Response_writer_ptr reswriter);

  /**
   * @brief      Returns the underlying HTTP header
   *
   * @return     A HTTP header
   */
  auto& header()
  { return reswriter_->header(); }

  const auto& header() const
  { return reswriter_->header(); }

  /**
   * @brief      Returns the underlying HTTP Response writer
   *
   * @return     A HTTP Response writer
   */
  auto& writer()
  { return *reswriter_; }

  /**
   * @brief      Returns the underlying HTTP Response object
   *
   * @return     The HTTP Response object
   */
  auto& source()
  { return reswriter_->response(); }

  /**
   * @brief      Returns the underlying HTTP Connection object
   *
   * @return     The HTTP Connection object
   */
  auto& connection()
  { return reswriter_->connection(); }

  /**
   * @brief      Send a HTTP Status code together with headers.
   *             Mostly used for replying with an error.
   *
   * @param[in]  <unnamed>  { parameter_description }
   * @param[in]  close      Whether to close the connection or not. Default = true
   */
  void send_code(const Code, bool close = true);

  /**
   * @brief      Send the underlying Response as is.
   *
   * @param[in]  force_close  whether to forcefully close the connection. Default = false
   */
  void send(bool force_close = false);

  /**
   * @brief      Send a file as a response
   *
   * @param[in]  file  The file to be sent
   */
  void send_file(const File& file);

  /**
   * @brief      Sends a response where the payload is json formatted data
   *
   * @param[in]  jsonstr  The json as a string
   */
  void send_json(const std::string& jsonstr);

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

  auto& writer_ptr()
  { return reswriter_; }

  ~Response();

private:
  http::Response_writer_ptr reswriter_;

}; // < class Response

inline Response::Error::Error()
  : code{http::Bad_Request}
{
}

inline Response::Error::Error(std::string&& type, std::string&& msg)
  : code{http::Bad_Request}, type{type}, message{msg}
{
}

inline Response::Error::Error(const Code code, std::string&& type, std::string&& msg)
  : code{code}, type{type}, message{msg}
{
}

inline std::string Response::Error::json() const
{
  return "{ \"type\" : \"" + type + "\", \"message\" : \"" + message + "\" }";
}

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
