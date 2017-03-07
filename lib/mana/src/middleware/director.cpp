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

#include <mana/middleware/director.hpp>

namespace mana {
namespace middleware {

const std::string Director::BOOTSTRAP_CSS = "<link rel=\"stylesheet\" href=\"//maxcdn.bootstrapcdn.com/bootstrap/3.3.6/css/bootstrap.min.css\" integrity=\"sha384-1q8mTJOASx8j1Au+a5WDVnPi2lkFfwwEAa8hDDdjZlpLegxhjVME1fgjWPGmkzs7\" crossorigin=\"anonymous\">";
const std::string Director::FONTAWESOME = "<link href=\"//maxcdn.bootstrapcdn.com/font-awesome/4.6.3/css/font-awesome.min.css\" rel=\"stylesheet\">";
const std::string Director::HTML_HEADER = "<html><head>" + BOOTSTRAP_CSS + FONTAWESOME + "</head><body>";
const std::string Director::HTML_FOOTER = "</body></html>";

void Director::process(
  mana::Request_ptr req,
  mana::Response_ptr res,
  mana::Next next
  )
{
  // get path
  std::string path = req->uri();

  auto fpath = resolve_file_path(path);
  #ifdef VERBOSE_WEBSERVER
  printf("<Director> Path: %s\n", fpath.c_str());
  #endif

  normalize_trailing_slashes(path);
  disk_->fs().ls(
    fpath,
    fs::on_ls_func::make_packed(
    [this, req, res, next, path](auto err, auto entries) {
      // Path not found on filesystem, go next
      if(err) {
        return (*next)();
      }
      else {
        res->source().add_body(create_html(entries, path));
        res->send();
      }
    })
  );
}

std::string Director::create_html(Entries entries, const std::string& path) {
  std::ostringstream ss;
  ss << HTML_HEADER;
  ss << "<div class=\"container\">";
  ss << "<h1 class=\"page-header\">" << path << "</h1>";
  ss << "<div class=\"panel panel-default\">";
  build_table(ss, entries, path);
  ss << "</div>"; // panel
  ss << "<hr/><h6 class=\"small\">Powered by <strong>IncludeOS</strong></h6>";
  ss << "</div>"; // container

  ss << HTML_FOOTER;
  return ss.str();
}

void Director::build_table(std::ostringstream& ss, Entries entries, const std::string& path) {
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

void Director::add_table_header(std::ostringstream& ss) {
  ss << "<tr>"
    << "<th>Type</th>"
    << "<th>Name</th>"
    << "<th>Size</th>"
    << "<th>Modified</th>"
    << "</tr>";
}

void Director::add_tr(std::ostringstream& ss, const std::string& path, const Entry& entry) {
  if(entry.name() == ".")
    return;

  bool isFile = entry.is_file();

  ss << "<tr>";
  add_td(ss, "type", get_icon(entry));
  add_td(ss, "file", create_url(path, entry.name()));
  isFile ? add_td(ss, "size", human_readable_size(entry.size())) : add_td(ss, "size", "-");
  isFile ? add_td(ss, "modified", "N/A") : add_td(ss, "modified", "-");
  ss << "</tr>";
}

std::string Director::get_icon(const Entry& entry) {
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

std::string Director::human_readable_size(double sz) const {
  const char* suffixes[] = {"B", "KB", "MB", "GB"};
  int i = 0;
  while(sz >= 1024 && ++i < 4)
    sz = sz/1024;

  char str[20];
  snprintf(&str[0], 20, "%.2f %s", sz, suffixes[i]);
  return str;
}

}} // < namespace mana::middleware
