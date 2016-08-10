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

#ifndef MIDDLEWARE_DIRECTOR_HPP
#define MIDDLEWARE_DIRECTOR_HPP

#include <sstream>

#include <fs/disk.hpp>

#include "middleware.hpp"

namespace middleware {

/**
 * @brief Responsible to set the scene of a directory.
 * @details Creates a simple html display of a directory entry on a IncludeOS disk.
 *
 */
class Director : public server::Middleware {
private:
  using SharedDisk = fs::Disk_ptr;
  using Entry = fs::Dirent;
  using Entries = fs::FileSystem::dirvec_t;

public:

  const static std::string HTML_HEADER;
  const static std::string HTML_FOOTER;
  const static std::string BOOTSTRAP_CSS;
  const static std::string FONTAWESOME;

  Director(SharedDisk disk, std::string root)
    : disk_(disk), root_(root) {}

  virtual void process(
    server::Request_ptr req,
    server::Response_ptr res,
    server::Next next
    ) override
  {
    printf("<Director> next=%ld\n", next.use_count());
    // get path
    std::string path = req->uri().path();

    auto fpath = resolve_file_path(path);

    printf("<Director> Path: %s\n", fpath.c_str());

    normalize_trailing_slashes(path);
    disk_->fs().ls(fpath, [this, req, res, next, path](auto err, auto entries) {
      printf("<Director> next=%ld\n", next.use_count());
      // Path not found on filesystem, go next
      if(err) {
        return (*next)();
      }
      else {
        res->add_body(create_html(entries, path));
        res->send();
      }
    });
  }

  virtual void onMount(const std::string& path) override {
    Middleware::onMount(path);
    printf("<Director> Mounted on [ %s ]\n", path.c_str());
  }

private:
  SharedDisk disk_;
  std::string root_;

  std::string create_html(Entries entries, const std::string& path) {
    std::ostringstream ss;
    ss << HTML_HEADER;
    ss << "<div class=\"container\">";
    ss << "<h1 class=\"page-header\">" << path << "</h1>";
    ss << "<div class=\"panel panel-default\">";
    build_table(ss, entries, path);
    ss << "</div>"; // panel
    ss << "<hr/><h6 class=\"small\">Powered by <strong>IncludeOS</strong> ;-)</h6>";
    ss << "</div>"; // container

    ss << HTML_FOOTER;
    return ss.str();
  }

  void build_table(std::ostringstream& ss, Entries entries, const std::string& path) {
    ss << "<table class=\"table table-hover\">";
    ss << "<thead>";
    add_table_header(ss);
    ss << "</thead>";

    ss << "<tbody>";
    for(auto e : *entries) {
      add_tr(ss, path, e);
    }
    ss << "</tbody>";

    ss << "</table>";
  }

  void add_table_header(std::ostringstream& ss) {
    ss << "<tr>"
      << "<th>Type</th>"
      << "<th>Name</th>"
      << "<th>Size(B)</th>"
      << "<th>Modified</th>"
      << "</tr>";
  }

  void add_tr(std::ostringstream& ss, const std::string& path, const Entry& entry) {
    if(entry.name() == ".")
      return;

    bool isFile = entry.is_file();

    ss << "<tr>";
    add_td(ss, "type", get_icon(entry));
    add_td(ss, "file", create_url(path, entry.name()));
    isFile ? add_td(ss, "size", entry.size()) : add_td(ss, "size", "-");
    isFile ? add_td(ss, "modified", entry.timestamp) : add_td(ss, "modified", "-");
    ss << "</tr>";
  }

  std::string get_icon(const Entry& entry) {
    std::ostringstream ss;
    ss << "<i class=\"fa fa-";
    if(entry.name() == "..") {
      ss << "level-up";
    }
    else if(entry.is_file()) {
      ss << "file";
    }
    else if(entry.is_dir()) {
      ss << "folder";
    }
    else {
      ss << "question";
    }
    ss << "\" aria-hidden=\"true\"></i>";
    return ss.str();
  }

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

}; // < class Director

const std::string Director::BOOTSTRAP_CSS = "<link rel=\"stylesheet\" href=\"//maxcdn.bootstrapcdn.com/bootstrap/3.3.6/css/bootstrap.min.css\" integrity=\"sha384-1q8mTJOASx8j1Au+a5WDVnPi2lkFfwwEAa8hDDdjZlpLegxhjVME1fgjWPGmkzs7\" crossorigin=\"anonymous\">";
const std::string Director::FONTAWESOME = "<link href=\"//maxcdn.bootstrapcdn.com/font-awesome/4.6.3/css/font-awesome.min.css\" rel=\"stylesheet\">";
const std::string Director::HTML_HEADER = "<html><head>" + BOOTSTRAP_CSS + FONTAWESOME + "</head><body>";
const std::string Director::HTML_FOOTER = "</body></html>";

} //< namespace middleware

#endif
