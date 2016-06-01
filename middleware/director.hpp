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
    std::string path = req->uri().path();

    auto fpath = resolve_file_path(path);

    printf("<Director> Path: %s\n", fpath.c_str());

    normalize_trailing_slashes(path);
    disk_->fs().ls(fpath, [this, req, res, next, path](auto err, auto entries) {
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

  std::string resolve_file_path(std::string path) {
    path.replace(0,mountpath_.size(), root_);
    return path;
  }

}; // < class Director

const std::string Director::HTML_HEADER = "<html><body><ul>";
const std::string Director::HTML_FOOTER = "</ul></body></html>";

#endif
