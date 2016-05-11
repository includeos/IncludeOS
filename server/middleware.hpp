#ifndef SERVER_MIDDLEWARE_HPP
#define SERVER_MIDDLEWARE_HPP

#include "request.hpp"
#include "response.hpp"


namespace server {

using Next = delegate<void()>;
using Callback = delegate<void(Request_ptr, Response_ptr, Next)>;

class Middleware {
public:

  virtual void process(Request_ptr, Response_ptr, Next) = 0;

  Callback callback() {
    return Callback::from<Middleware, &Middleware::process>(this);
  }

  operator Callback () {
    return callback();
  }
};

}; // << namespace server


#endif
