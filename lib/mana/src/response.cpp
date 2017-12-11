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

#include <mana/response.hpp>
#include <util/async.hpp>

using namespace mana;
using namespace std::string_literals;

Response::Response(http::Response_writer_ptr res)
  : reswriter_(std::move(res))
{
}

void Response::send(bool force_close)
{
  if(force_close)
    header().set_field(http::header::Connection, "close");

  reswriter_->write();
}

void Response::send_code(const Code code, bool force_close)
{
  if(force_close)
    header().set_field(http::header::Connection, "close");

  reswriter_->write_header(code);
}

void Response::send_file(const File& file)
{
  auto& entry = file.entry;

  /* Content Length */
  header().set_content_length(entry.size());

  /* Send header */
  reswriter_->write_header(http::OK);

  /* Send file over connection */
  auto* stream = reswriter_->connection().stream().get();
  #ifdef VERBOSE_WEBSERVER
  printf("<Response> Sending file: %s (%llu B).\n",
    entry.name().c_str(), entry.size());
  #endif

  Async::upload_file(
    file.disk,
    file.entry,
    stream,
    Async::on_after_func::make_packed(
    [
      stream,
      req{shared_from_this()}, // keep the response (and conn) alive until done
      entry
    ] (fs::error_t err, bool good) mutable
    {
      if(good) {
        #ifdef VERBOSE_WEBSERVER
        printf("<Response> Success sending %s => %s\n",
          entry.name().c_str(), stream->remote().to_string().c_str());
        #endif
      }
      else {
        printf("<Response> Error sending %s => %s [%s]\n",
          entry.name().c_str(), stream->remote().to_string().c_str(),
          stream->is_closing() ? "Connection closing" : err.to_string().c_str());
      }
      // remove on_write triggering for other
      // writes on the same connection
      stream->on_write(nullptr);
    })
  );
}

void Response::send_json(const std::string& json)
{
  header().set_field(http::header::Content_Type, "application/json");
  header().set_content_length(json.size());
  // be simple for now
  source().add_body(json);
  send();
}

void Response::error(Error&& err) {
  // NOTE: only cares about JSON (for now)
  source().set_status_code(err.code);
  send_json(err.json());
}

Response::~Response() {
  #ifdef VERBOSE_WEBSERVER
  printf("<Response> Deleted (%s)\n", conn_->to_string().c_str());
  #endif
}
