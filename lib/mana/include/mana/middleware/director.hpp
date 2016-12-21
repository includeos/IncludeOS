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

#ifndef MANA_MIDDLEWARE_DIRECTOR_HPP
#define MANA_MIDDLEWARE_DIRECTOR_HPP

#include <sstream>
#include <fs/disk.hpp>

#include <mana/middleware.hpp>

namespace mana {
namespace middleware {

/**
 * @brief Responsible to set the scene of a directory.
 * @details Creates a simple html display of a directory entry on a IncludeOS disk.
 *
 */
class Director : public Middleware {
private:
  using SharedDisk = fs::Disk_ptr;
  using Entry = fs::Dirent;
  using Entries = fs::dirvec_t;

public:

  const static std::string HTML_HEADER;
  const static std::string HTML_FOOTER;
  const static std::string BOOTSTRAP_CSS;
  const static std::string FONTAWESOME;

  Director(SharedDisk disk, std::string root)
    : disk_(disk), root_(root) {}

  Callback handler() override {
    return {this, &Director::process};
  }

  void process(mana::Request_ptr req, mana::Response_ptr res, mana::Next next);

  void on_mount(const std::string& path) override {
    Middleware::on_mount(path);
    printf("<Director> Mounted on [ %s ]\n", path.c_str());
  }

private:
  SharedDisk disk_;
  std::string root_;

  std::string create_html(Entries entries, const std::string& path);

  void build_table(std::ostringstream& ss, Entries entries, const std::string& path);

  void add_table_header(std::ostringstream& ss);

  void add_tr(std::ostringstream& ss, const std::string& path, const Entry& entry);

  std::string get_icon(const Entry& entry);

  std::string create_url(const std::string& path, const std::string& name) {
    return "<a href=\"" + path + name + "\">" + name + "</a>";
  }

  template <typename T>
  void add_td(std::ostringstream& ss, std::string&& cl, const T& field) {
    ss << "<td class=\"" << cl << "\">" << field << "</td>";
  }

  void normalize_trailing_slashes(std::string& path) {
    if(!path.empty() && path.back() != '/')
      path += '/';
  }

  std::string resolve_file_path(std::string path) {
    path.replace(0,mountpath_.size(), root_);
    return path;
  }

  std::string human_readable_size(double sz) const;

}; // < class Director

}} //< namespace mana::middleware

#endif
