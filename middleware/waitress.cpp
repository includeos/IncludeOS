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

#ifndef MIDDLEWARE_WAITRESS_HPP
#define MIDDLEWARE_WAITRESS_HPP

#include "middleware.hpp" // inherit middleware
#include <fs/disk.hpp>
#include <string>

namespace middleware {

/**
 * @brief Serves files (not food)
 * @details Serves files from a IncludeOS disk.
 *
 */
class Waitress : public server::Middleware {
private:
  using SharedDisk = std::shared_ptr<fs::Disk>;
  using Entry = fs::FileSystem::Dirent;
  using OnStat = fs::FileSystem::on_stat_func;

public:

  struct Options {
    std::vector<const char*> index;
    bool fallthrough;

    Options() = default;
    Options(std::initializer_list<const char*> indices, bool fallth = true)
      : index(indices), fallthrough(fallth) {}
  };

  Waitress(SharedDisk disk, std::string root, Options opt = {{"index.html"}, true})
    : disk_(disk), root_(root), options_(opt) {}

  virtual void process(
    server::Request_ptr req,
    server::Response_ptr res,
    server::Next next
    ) override
  {
    // if not a valid request
    if(req->method() != http::GET && req->method() != http::HEAD) {
      if(options_.fallthrough) {
        return (*next)();
      }

      // TODO: Want to be able to write "GET, HEAD" instead of std::string{"GET, HEAD"}:
      res->add_header(http::header_fields::Entity::Allow, std::string{"GET, HEAD"});
      return res->send_code(http::Method_Not_Allowed);
    }

    // get path
    std::string path = req->uri().path();
    // resolve extension
    auto ext = get_extension(path);
    // concatenate root with path, example: / => /public/
    path = root_ + path;


    // no extension found
    if(ext.empty()) {
      // lets try to see if we can serve an index
      path += options_.index[0]; // only check first one for now, else we have to use fs().ls
      disk_->fs().stat(path, [this, req, res, next, path](auto err, const auto& entry) {
        // no index was found on this path, go to next middleware
        if(err) {
          return (*next)();
        }
        // we got an index, lets send it
        else {
          http::Mime_Type mime = http::extension_to_type(get_extension(path));
          res->add_header(http::header_fields::Entity::Content_Type, mime);
          return res->send_file({disk_, entry});
        }
      });
    }
    // we found an extension, this is a (probably) a file request
    else {
      //printf("<Waitress> Extension found - assuming request for file.\n");
      disk_->fs().stat(path, [this, req, res, next, path](auto err, const auto& entry) {
        if(err) {
          printf("<Waitress> File not found. Replying with 404.\n");
          return res->send_code(http::Not_Found);
          /*
          if(!options_.fallthrough) {
            printf("<Waitress> File not found. Replying with 404.\n");
            return res->send_code(http::Not_Found);
          }
          else {
            return (*next)();
          }*/
        }
        else {
          printf("<Waitress> Found file: %s (%llu B)\n", entry.name().c_str(), entry.size());
          http::Mime_Type mime = http::extension_to_type(get_extension(path));
          res->add_header(http::header_fields::Entity::Content_Type, mime);
          return res->send_file({disk_, entry});
        }
      });
    }
  }



private:
  SharedDisk disk_;
  std::string root_;
  Options options_;

  std::string get_extension(const std::string& path) const {
    std::string ext;
    auto idx = path.find_last_of(".");
    // Find extension
    if(idx != std::string::npos) {
      ext = path.substr(idx+1);
    }
    return ext;
  }

  inline bool allowed_extension(const std::string& extension) const {
    return !extension.empty();
  }

  /**
   * @brief Check if the path contains a file request
   * @details Very bad but easy way to assume the path is a request for a file.
   *
   * @param path
   * @return wether a path is a file request or not
   */
  inline bool is_file_request(const std::string& path) const
  { return !get_extension(path).empty(); }

}; // < class Waitress

} //< namespace middleware

#endif
