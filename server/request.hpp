#ifndef SERVER_REQUEST_HPP
#define SERVER_REQUEST_HPP

#include "http/inc/request.hpp"

namespace server {

class Request;
using Request_ptr = std::shared_ptr<Request>;

class Request : public http::Request {
private:
  using Parent = http::Request;
  using buffer_t = std::shared_ptr<uint8_t>;

public:
  // inherit constructors
  using Parent::Parent;

  Request(buffer_t, size_t);

}; // < server::Request

}; // < namespace server



#endif
