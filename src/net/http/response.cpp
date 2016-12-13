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

#include <http-parser/http_parser.h>
#include <net/http/response.hpp>

namespace http {

static http_parser_settings settings;

static void configure_settings(http_parser_settings&) noexcept;

static size_t execute_parser(Response*, http_parser&, http_parser_settings&, const std::string&) noexcept;

///////////////////////////////////////////////////////////////////////////////
Response::Response(const Version version, const status_t status_code) noexcept
  : code_{status_code}
  , version_{version}
{
  configure_settings(settings);
}

///////////////////////////////////////////////////////////////////////////////
Response::Response(std::string response, const std::size_t limit, const bool parse)
  : Message{limit}
  , response_{std::move(response)}
{
  configure_settings(settings);

  if (parse) this->parse();
}

///////////////////////////////////////////////////////////////////////////////
Response& Response::parse() {
  http_parser parser;

  soft_reset();

  if (execute_parser(this, parser, settings, response_) not_eq response_.length()) {
    throw Response_error{"Invalid response: " + response_};
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////
status_t Response::status_code() const noexcept {
  return code_;
}

///////////////////////////////////////////////////////////////////////////////
Response& Response::set_status_code(const status_t status_code) noexcept {
  code_ = status_code;
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
const Version Response::version() const noexcept {
  return version_;
}

///////////////////////////////////////////////////////////////////////////////
Response& Response::set_version(const Version version) noexcept {
  version_ = version;
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
Response& Response::reset() noexcept {
  response_.clear();
  return soft_reset();
}

///////////////////////////////////////////////////////////////////////////////
std::string Response::to_string() const {
  std::ostringstream response;
  //-----------------------------------
  response << version_ << " " << code_ << " "
           << code_description(code_)  << "\r\n"
           << Message::to_string();
  //-----------------------------------
  return response.str();
}

///////////////////////////////////////////////////////////////////////////////
Response::operator std::string () const {
  return to_string();
}

///////////////////////////////////////////////////////////////////////////////
static void configure_settings(http_parser_settings& settings_) noexcept {
  http_parser_settings_init(&settings_);

  settings_.on_header_field = [](http_parser* parser, const char* at, size_t length) {
    auto res = reinterpret_cast<Response*>(parser->data);
    res->set_private_field(at, length);
    return 0;
  };

  settings_.on_header_value = [](http_parser* parser, const char* at, size_t length) {
    auto res = reinterpret_cast<Response*>(parser->data);
    res->header().set_field(res->private_field().to_string(), {at, length});
    return 0;
  };

  settings_.on_body = [](http_parser* parser, const char* at, size_t length) {
    auto res = reinterpret_cast<Response*>(parser->data);
    res->add_body({at, length});
    return 0;
  };

  settings_.on_headers_complete = [](http_parser* parser) {
    auto res = reinterpret_cast<Response*>(parser->data);
    res->set_version(Version{parser->http_major, parser->http_minor});
    res->set_status_code(static_cast<status_t>(parser->status_code));
    return 0;
  };
}

///////////////////////////////////////////////////////////////////////////////
static size_t execute_parser(Response* res, http_parser& parser, http_parser_settings& settings,
                           const std::string& data) noexcept {
  http_parser_init(&parser, HTTP_RESPONSE);
  parser.data = res;
  return http_parser_execute(&parser, &settings, data.data(), data.size());
}

///////////////////////////////////////////////////////////////////////////////
Response& Response::operator << (const std::string& chunk) {
  response_.append(chunk);
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
Response& Response::soft_reset() noexcept {
  Message::reset();
  return set_status_code(OK);
}

} //< namespace http
