
#ifndef SERVER_RESPONSE_HPP
#define SERVER_RESPONSE_HPP

#include "http/inc/response.hpp"
#include "http/inc/mime_types.hpp"
#include <fs/filesystem.hpp>
#include <net/tcp.hpp>
#include <utility/async.hpp>

struct File {

  File(fs::Disk_ptr dptr, const fs::Dirent& ent)
    : disk(dptr)
  {
    assert(ent.is_file());
    entry = ent;
  }

  fs::Dirent entry;
  fs::Disk_ptr disk;
};

namespace server {

class Response;
using Response_ptr = std::shared_ptr<Response>;

class Response : public http::Response {
private:
  using Code = http::status_t;
  using Connection_ptr = net::TCP::Connection_ptr;

public:

  Response(Connection_ptr conn);

  /*
    Send only status code
  */
  void send_code(const Code);

  /*
    Send the Response
  */
  void send(bool close = false) const;

  /*
    Send a file
  */
  void send_file(const File&);

  void send_json(const std::string&);

  /*
    "End" the response
  */
  void end() const;

private:
  Connection_ptr conn_;

  bool keep_alive = true;

  void write_to_conn(bool close_on_written = false) const;

}; // server::Response


} // < server

#endif
