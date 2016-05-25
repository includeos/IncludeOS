#ifndef SERVER_MIDDLEWARE_HPP
#define SERVER_MIDDLEWARE_HPP

#include "request.hpp"
#include "response.hpp"


namespace server {

class Middleware;
using Middleware_ptr = std::shared_ptr<Middleware>;

using next_t = delegate<void()>;
using Next = std::shared_ptr<next_t>;
using Callback = delegate<void(Request_ptr, Response_ptr, Next)>;

class Middleware {
public:

  virtual void process(Request_ptr, Response_ptr, Next) = 0;

  Callback callback() {
    return Callback::from<Middleware, &Middleware::process>(this);
  }

  virtual void onMount(const std::string&) {
    // do nothing
  }

};

}; // << namespace server


#endif
