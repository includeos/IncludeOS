#include "response.hpp"

using namespace server;

Response::Response(Connection_ptr conn)
  : http::Response(), conn_(conn)
{
  add_header(http::header_fields::Response::Server, "IncludeOS/Acorn");
  // screw keep alive
  add_header(http::header_fields::Response::Connection, "keep-alive");
}

void Response::send(bool close) const {
  write_to_conn(close);
  end();
}

void Response::write_to_conn(bool close_on_written) const {
  auto res = to_string();

  if(!close_on_written) {
    conn_->write(res.data(), res.size());
  }
  else {
    auto conn = conn_;
    conn_->write(res.data(), res.size(), [conn](size_t) {
      conn->close();
    });
  }
}

void Response::send_code(const Code code) {
  set_status_code(code);
  send(!keep_alive);
}

void Response::send_file(const File& file) {
  auto& entry = file.entry;

  /* Content Length */
  add_header(http::header_fields::Entity::Content_Length, std::to_string(entry.size()));

  /* Send header */
  auto res = to_string();
  conn_->write(res.data(), res.size());

  /* Send file over connection */
  auto conn = conn_;
  printf("<Response> Sending file: %s (%llu B).\n",
    entry.name().c_str(), entry.size());

  Async::upload_file(file.disk, file.entry, conn,
    [conn, entry](fs::error_t err, bool good)
  {
      if(good) {
        printf("<Response> Success sending %s => %s\n",
          entry.name().c_str(), conn->remote().to_string().c_str());
        //conn->close();
      }
      else {
        printf("<Response> Error sending %s => %s [%s]\n",
          entry.name().c_str(), conn->remote().to_string().c_str(), err.to_string().c_str());
      }
  });

  end();
}

void Response::send_json(const std::string& json) {
  add_body(json);
  add_header(http::header_fields::Entity::Content_Type, "application/json");
  send(!keep_alive);
}

void Response::error(Error&& err) {
  // NOTE: only cares about JSON (for now)
  set_status_code(err.code);
  send_json(err.json());
}

void Response::end() const {
  // Response ended, signal server?
}
