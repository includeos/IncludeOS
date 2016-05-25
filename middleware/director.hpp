#ifndef MIDDLEWARE_DIRECTOR_HPP
#define MIDDLEWARE_DIRECTOR_HPP

#include "middleware.hpp" // inherit middleware
#include <fs/disk.hpp>
#include <sstream>


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

  Director(SharedDisk disk, std::string root)
    : disk_(disk), root_(root) {}

  virtual void process(
    server::Request_ptr req,
    server::Response_ptr res,
    server::Next next
    ) override
  {
    // get path
    std::string org_path = req->uri().path();
    normalize_trailing_slashes(org_path);
    auto path = root_ + org_path;

    printf("<Director> Path: %s\n", path.c_str());

    disk_->fs().ls(path, [this, req, res, next, org_path](auto err, auto entries) {
      // Path not found on filesystem, go next
      if(err) {
        return (*next)();
      }
      else {
        res->add_body(create_html(entries, org_path));
        res->send();
      }
    });
  }

private:
  SharedDisk disk_;
  std::string root_;

  std::string create_html(Entries entries, const std::string& path) {
    std::ostringstream ss;
    ss << HTML_HEADER;

    for(auto e : *entries) {
      create_li(ss, path, e);
    }

    ss << HTML_FOOTER;
    return ss.str();
  }

  void create_li(std::ostringstream& ss, const std::string& path, const Entry& entry) {
    ss << "<li><a href=\"" << path << entry.name() << "\">"
      << entry.name() << "</a></li>";
  }

  void normalize_trailing_slashes(std::string& path) {
    if(!path.empty() && path.back() != '/')
      path += '/';
  }

}; // < class Director

const std::string Director::HTML_HEADER = "<html><body><ul>";
const std::string Director::HTML_FOOTER = "</ul></body></html>";

#endif
