
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

class ServerResponse : public http::Response {
private:
  using Code = http::status_t;
  using Connection_ptr = net::TCP::Connection_ptr;

public:

  ServerResponse(Connection_ptr conn);

  /*
    Send only status code
  */
  void send_code(const Code);

  /*
    Send the Response
  */
  void send() const;

  /*
    Send a file
  */
  void send_file(const File&);

  /*
    "End" the response
  */
  void end() const;

private:
  Connection_ptr conn_;

};

ServerResponse::ServerResponse(Connection_ptr conn)
  : http::Response(), conn_(conn)
{
  add_header(http::header_fields::Response::Server, "IncludeOS/Acorn");
  // screw keep alive
  add_header(http::header_fields::Response::Connection, "close");
}

void ServerResponse::send() const {
  auto res = to_string();
  conn_->write(res.data(), res.size());
  end();
}

void ServerResponse::send_code(const Code code) {
  set_status_code(code);
  send();
}

void ServerResponse::send_file(const File& file) {
  auto& fname = file.entry.fname;

  /* Content Length */
  add_header(http::header_fields::Entity::Content_Length, std::to_string(file.entry.size()));

  /* Setup MIME type */
  std::string ext = "txt"; // fallback
  http::Mime_Type mime;
  // find dot before extension
  auto ext_i = fname.find_last_of(".");
  // extension found
  if(ext_i != std::string::npos) {
    // get the extension
    ext = fname.substr(ext_i + 1);
  }
  // find the correct mime type
  mime = http::extension_to_type(ext);

  add_header(http::header_fields::Entity::Content_Type, mime);

  /* Send header */
  auto res = to_string();
  conn_->write(res.data(), res.size());

  /* Send file over connection */
  auto conn = conn_;
  printf("<ServerResponse::send_file> Asking to send %llu bytes.\n", file.entry.size());
  Async::upload_file(file.disk, file.entry, conn,
    [conn](fs::error_t err, bool good)
  {
      if(good) {
        printf("<ServerResponse::send_file> %s - Success!\n",
          conn->to_string().c_str());
        //conn->close();
      }
      else {
        printf("<ServerResponse::send_file> %s - Error: %s\n",
          conn->to_string().c_str(), err.to_string().c_str());
      }
  });

  end();
}

void ServerResponse::end() const {
  // Response ended, signal server?
}

#endif
