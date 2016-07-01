#include "request.hpp"

using namespace server;

Request::OnRecv Request::on_recv_ = [](size_t) {};

Request::Request(buffer_t buf, size_t n)
  : Parent(std::string{(char*)buf.get(), n})
{

}

void Request::complete() {
  assert(is_complete());
  on_recv_(total_length());
}

size_t Request::content_length() const {
  using namespace http::header_fields::Entity;
  if(!has_header(Content_Length))
    return 0;
  try {
    return std::stoull(header_value(Content_Length));
  }
  catch(...) {
    return 0;
  }
}

Request::~Request() {
  //printf("<Request> Deleted\n");
}
