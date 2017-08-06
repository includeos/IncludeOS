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

#ifndef MANA_MIDDLEWARE_BUTLER_HPP
#define MANA_MIDDLEWARE_BUTLER_HPP

#include <mana/middleware.hpp> // inherit middleware
#include <fs/disk.hpp>
#include <string>

namespace mana {
namespace middleware {

/**
 * @brief Serves files (not food)
 * @details Serves files from a IncludeOS disk.
 *
 */
class Butler : public Middleware {
private:
  using SharedDisk  = std::shared_ptr<fs::Disk>;
  using Entry       = fs::Dirent;
  using OnStat       = fs::on_stat_func;

public:

  struct Options {
    std::vector<const char*> index;
    bool fallthrough;

    Options() = default;
    Options(std::initializer_list<const char*> indices, bool fallth = true)
      : index(indices), fallthrough(fallth) {}
  };

  Butler(SharedDisk disk, std::string root, Options opt = {{"index.html"}, true});

  Callback handler() override {
    return {this, &Butler::process};
  }

  void process(Request_ptr req, Response_ptr res, Next next);

private:
  SharedDisk disk_;
  std::string root_;
  Options options_;

  std::string get_extension(const std::string& path) const;

  inline bool allowed_extension(const std::string& extension) const {
    return !extension.empty();
  }

  /**
   * @brief Check if the path contains a file request
   * @details Very bad but easy way to assume the path is a request for a file.
   *
   * @param path
   * @return whether a path is a file request or not
   */
  inline bool is_file_request(const std::string& path) const
  { return !get_extension(path).empty(); }

}; // < class Butler

}} //< namespace mana::middleware

#endif
