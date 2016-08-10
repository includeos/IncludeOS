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

#include "response.hpp"

using namespace server;

Response::OnSent Response::on_sent_ = [](size_t){};

Response::Response(Connection_ptr conn)
  : http::Response(), conn_(conn)
{
  // TODO: Want to be able to write "GET, HEAD" instead of std::string{"..."}:
  add_header(http::header_fields::Response::Server, std::string{"IncludeOS/Acorn"});

  // screw keep alive

  // TODO: Want to be able to write "GET, HEAD" instead of std::string{"..."}:
  add_header(http::header_fields::Response::Connection, std::string{"keep-alive"});
}

void Response::send(bool close) const {
  write_to_conn(close);
  end();
}

void Response::write_to_conn(bool close_on_written) const {
  auto res = to_string();
  auto conn = conn_;
  conn_->write(res.data(), res.size(),
    [conn, close_on_written](size_t n) {
      on_sent_(n);
      if(close_on_written)
        conn->close();
    });
}

void Response::send_code(const Code code) {
  set_status_code(code);
  send(!keep_alive);
}

void Response::send_file(const File& file) {
  auto& entry = file.entry;

  /* Content Length */
  add_header(http::header_fields::Entity::Content_Length, std::to_string(entry.size()));

  /* Send header */
  auto res = to_string();
  conn_->write(res.data(), res.size());

  /* Send file over connection */
  auto conn = conn_;
  #ifdef VERBOSE_WEBSERVER
  printf("<Response> Sending file: %s (%llu B).\n",
    entry.name().c_str(), entry.size());
  #endif

  //auto buffer = file.disk->fs().read(entry, 0, entry.size());
  //printf("<Respone> Content:%.*s\n", buffer.size(), buffer.data());

  Async::upload_file(file.disk, file.entry, conn,
    [conn, entry](fs::error_t err, bool good)
  {
      if(good) {
        #ifdef VERBOSE_WEBSERVER
        printf("<Response> Success sending %s => %s\n",
          entry.name().c_str(), conn->remote().to_string().c_str());
        #endif

        on_sent_(entry.size());
      }
      else {
        printf("<Response> Error sending %s => %s [%s]\n",
          entry.name().c_str(), conn->remote().to_string().c_str(), err.to_string().c_str());
      }
  });

  end();
}

void Response::send_json(const std::string& json) {
  add_body(json);

  // TODO: Want to be able to write "GET, HEAD" instead of std::string{"..."}:
  add_header(http::header_fields::Entity::Content_Type, std::string{"application/json"});

  send(!keep_alive);
}

/* Cookie-support start */

void Response::cookie(const Cookie& c) {
  add_header(http::header_fields::Response::Set_Cookie, c.to_string());
  send(keep_alive);
}

void Response::cookie(const std::string& name, const std::string& value) {

  // Can throw CookieException

  Cookie c{name, value};
  cookie(c);
}

void Response::cookie(const std::string& name, const std::string& value, const std::vector<std::string>& options) {

  // Can throw CookieException

  Cookie c{name, value, options};
  cookie(c);
}

void Response::update_cookie(const std::string& name, const std::string& old_path, const std::string& old_domain,
  const std::string& new_value) {

  // Can throw CookieException

  // 1. Clear old cookie:

  Cookie c{name, ""};
  c.set_path(old_path);
  c.set_domain(old_domain);
  c.set_expires("Sun, 06 Nov 1994 08:49:37 GMT"); // in the past

  add_header(http::header_fields::Response::Set_Cookie, c.to_string());

  // 2. Set new cookie:

  Cookie new_cookie{name, new_value};
  add_header(http::header_fields::Response::Set_Cookie, new_cookie.to_string());

  // 3. Send:

  send(keep_alive);
}

void Response::update_cookie(const std::string& name, const std::string& old_path, const std::string& old_domain,
  const std::string& new_value, const std::vector<std::string>& new_options) {

  // Can throw CookieException

  // 1. Clear old cookie:

  Cookie c{name, ""};
  c.set_path(old_path);
  c.set_domain(old_domain);
  c.set_expires("Sun, 06 Nov 1994 08:49:37 GMT"); // in the past

  add_header(http::header_fields::Response::Set_Cookie, c.to_string());

  // 2. Set new cookie:

  Cookie new_cookie{name, new_value, new_options};
  add_header(http::header_fields::Response::Set_Cookie, new_cookie.to_string());

  // 3. Send:

  send(keep_alive);
}

void Response::clear_cookie(const std::string& name, const std::string& path, const std::string& domain) {

  // Can throw CookieException

  Cookie c{name, ""};
  c.set_path(path);
  c.set_domain(domain);
  c.set_expires("Sun, 06 Nov 1994 08:49:37 GMT"); // in the past

  cookie(c);
}

/* Cookie-support end */

void Response::error(Error&& err) {
  // NOTE: only cares about JSON (for now)
  set_status_code(err.code);
  send_json(err.json());
}

void Response::end() const {
  // Response ended, signal server?
}

Response::~Response() {
  //printf("<Response> Deleted\n");
}
