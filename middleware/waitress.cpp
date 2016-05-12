#ifndef MIDDLEWARE_WAITRESS_HPP
#define MIDDLEWARE_WAITRESS_HPP

#include "middleware.hpp" // inherit middleware
#include <fs/disk.hpp>


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
  Waitress(SharedDisk disk) : disk_(disk) {}

  virtual void process(
    server::Request_ptr req,
    server::Response_ptr res,
    server::Next next
    ) override
  {
    // get path
    std::string path = req->uri().path();

    // check if request is for a file
    if(!is_file_request(path)) {
      printf("<Waitress> No extension (%s) - This is not a request for a file. (next is called)\n", path.c_str());
      (*next)();
      return;
    }

    printf("<Waitress> Extension found - assuming request for file.\n");
    disk_->fs().stat(path, [this, req, res, next](auto err, const auto& entry) {
      if(err) {
        printf("<Waitress> File not found. Replying with 404.\n");
        res->send_code(http::Not_Found);
      }
      else {
        printf("<Waitress> Found file: %s (%llu B)\n", entry.name().c_str(), entry.size());

        http::Mime_Type mime = http::extension_to_type(get_extension(req->uri().path()));

        res->add_header(http::header_fields::Entity::Content_Type, mime);
        res->send_file({disk_, entry});
      }
    });
  }

private:
  SharedDisk disk_;

  std::string get_extension(const std::string& path) const {
    std::string ext;
    auto idx = path.find_last_of(".");
    // Find extension
    if(idx != std::string::npos) {
      ext = path.substr(idx+1);
    }
    return ext;
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


#endif
