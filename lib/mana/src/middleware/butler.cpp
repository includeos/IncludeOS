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

#include <mana/middleware/butler.hpp>
#include <net/http/mime_types.hpp>

using namespace std::string_literals;

namespace mana {
namespace middleware {

Butler::Butler(SharedDisk disk, std::string root, Options opt)
  : disk_(disk), root_(root), options_(opt)
{
  Expects(disk != nullptr && disk->fs_ready());
}

void Butler::process(mana::Request_ptr req, mana::Response_ptr res, mana::Next next)
{
  // if not a valid request
  if(req->method() != http::GET && req->method() != http::HEAD) {
    if(options_.fallthrough) {
      return (*next)();
    }
    res->header().set_field(http::header::Allow, "GET, HEAD"s);
    res->send_code(http::Method_Not_Allowed);
    return;
  }

  // get path
  std::string path(req->uri());
  // resolve extension
  auto ext = get_extension(path);
  // concatenate root with path, example: / => /public/
  path = root_ + path;

  // no extension found
  if(ext.empty() and !options_.index.empty()) {
    if(path.back() != '/') path += '/';
    // lets try to see if we can serve an index
    path += options_.index.at(0); // only check first one for now, else we have to use fs().ls
    disk_->fs().cstat(
      path,
      fs::on_stat_func::make_packed(
      [this, req, res, next, path](auto err, const auto& entry)
      {
        //printf("<Butler> err=%s path=%s entry=%s\n",
        //  err.to_string().c_str(), path.c_str(), entry.name().c_str());
        // no index was found on this path, go to next middleware
        if(err or !entry.is_file()) {
          return (*next)();
        }
        // we got an index, lets send it
        else {
          auto mime = http::ext_to_mime_type(this->get_extension(path));
          res->header().set_field(http::header::Content_Type, std::string(mime));
          return res->send_file({disk_, entry});
        }
      })
    );
  }
  // we found an extension, this is a (probably) a file request
  else {
    //printf("<Butler> Extension found - assuming request for file.\n");
    disk_->fs().cstat(
      path,
      fs::on_stat_func::make_packed(
      [this, req, res, next, path](auto err, const auto& entry)
      {
        //printf("<Butler> err=%s path=%s entry=%s\n",
        //  err.to_string().c_str(), path.c_str(), entry.name().c_str());
        if(err or !entry.is_file()) {
          #ifdef VERBOSE_WEBSERVER
          printf("<Butler> File not found. Replying with 404.\n");
          #endif
          res->send_code(http::Not_Found);
          return;
          /*
          if(!options_.fallthrough) {
            printf("<Butler> File not found. Replying with 404.\n");
            return res->send_code(http::Not_Found);
          }
          else {
            return (*next)();
          }*/
        }
        else {
          #ifdef VERBOSE_WEBSERVER
          printf("<Butler> Found file: %s (%llu B)\n", entry.name().c_str(), entry.size());
          #endif
          auto mime = http::ext_to_mime_type(this->get_extension(path));
          res->header().set_field(http::header::Content_Type, std::string(mime));
          res->send_file({disk_, entry});
          return;
        }
      })
    );
  }
}

std::string Butler::get_extension(const std::string& path) const {
  std::string ext;
  auto idx = path.find_last_of(".");
  // Find extension
  if(idx != std::string::npos) {
    ext = path.substr(idx+1);
  }
  return ext;
}


}} // < namespace mana::middleware
