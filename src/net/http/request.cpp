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
#include <net/http/request.hpp>

namespace http {

static http_parser_settings settings;

static void configure_settings(http_parser_settings&) noexcept;

static size_t execute_parser(Request*, http_parser&, http_parser_settings&, const std::string&) noexcept;

///////////////////////////////////////////////////////////////////////////////
Request::Request() { configure_settings(settings); }

///////////////////////////////////////////////////////////////////////////////
Request::Request(std::string request, const std::size_t limit, const bool parse)
  : Message{limit}
  , request_{std::move(request)}
{
  configure_settings(settings);

  if (parse) this->parse();
}

///////////////////////////////////////////////////////////////////////////////
Request& Request::parse() {
  http_parser parser;

  soft_reset();

  if (execute_parser(this, parser, settings, request_) not_eq request_.length()) {
    throw Request_error{"Invalid request: " + request_};
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////
Method Request::method() const noexcept {
  return method_;
}

///////////////////////////////////////////////////////////////////////////////
Request& Request::set_method(const Method method) {
  method_ = method;
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
const URI& Request::uri() const noexcept {
  return uri_;
}

///////////////////////////////////////////////////////////////////////////////
Request& Request::set_uri(const URI& uri) {
  uri_ = uri;
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
const Version& Request::version() const noexcept {
  return version_;
}

///////////////////////////////////////////////////////////////////////////////
Request& Request::set_version(const Version& version) noexcept {
  version_ = version;
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
Request& Request::reset() noexcept {
  request_.clear();
  return soft_reset();
}

///////////////////////////////////////////////////////////////////////////////
std::string Request::to_string() const {
  std::ostringstream request;
  //-----------------------------------
  request << method_ << " "      << uri_
          << " "     << version_ << "\r\n"
          << Message::to_string();
  //-----------------------------------
  return request.str();
}

///////////////////////////////////////////////////////////////////////////////
Request::operator std::string () const {
  return to_string();
}

///////////////////////////////////////////////////////////////////////////////
static void configure_settings(http_parser_settings& settings_) noexcept {
  http_parser_settings_init(&settings_);

  settings_.on_message_begin = [](http_parser* parser) {
    auto req = reinterpret_cast<Request*>(parser->data);
    req->set_method(
            http::method::code(
                http_method_str(static_cast<http_method>(parser->method))));
    return 0;
  };

  settings_.on_url = [](http_parser* parser, const char* at, size_t length) {
    auto req = reinterpret_cast<Request*>(parser->data);
    req->set_uri(URI{{at, length}});
    return 0;
  };

  settings_.on_header_field = [](http_parser* parser, const char* at, size_t length) {
    auto req = reinterpret_cast<Request*>(parser->data);
    req->set_private_field(at, length);
    return 0;
  };

  settings_.on_header_value = [](http_parser* parser, const char* at, size_t length) {
    auto req = reinterpret_cast<Request*>(parser->data);
    req->header().set_field(req->private_field().to_string(), {at, length});
    return 0;
  };

  settings_.on_body = [](http_parser* parser, const char* at, size_t length) {
    auto req = reinterpret_cast<Request*>(parser->data);
    req->add_chunk({at, length});
    return 0;
  };

  settings_.on_headers_complete = [](http_parser* parser) {
    auto req = reinterpret_cast<Request*>(parser->data);
    req->set_version(Version{parser->http_major, parser->http_minor});
    return 0;
  };
}

///////////////////////////////////////////////////////////////////////////////
static size_t execute_parser(Request* req, http_parser& parser, http_parser_settings& settings,
                           const std::string& data) noexcept {
  http_parser_init(&parser, HTTP_REQUEST);
  parser.data = req;
  return http_parser_execute(&parser, &settings, data.data(), data.size());
}

///////////////////////////////////////////////////////////////////////////////
Request& Request::operator << (const std::string& chunk) {
  request_.append(chunk);
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
Request& Request::soft_reset() noexcept {
  Message::reset();
  return set_method(GET)
        .set_uri(URI{"/"})
        .set_version(Version{1U, 1U});
}

} //< namespace http
