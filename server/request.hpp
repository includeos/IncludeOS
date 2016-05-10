#ifndef SERVER_REQUEST_HPP
#define SERVER_REQUEST_HPP


#include "http/inc/request.hpp"

class Request : public http::Request {
private:
  using Parent = http::Request;
public:
  // inherit constructors
  using Parent::Parent;
};


#endif
