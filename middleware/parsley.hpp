#ifndef MIDDLEWARE_PARSLEY_HPP
#define MIDDLEWARE_PARSLEY_HPP

#include "json.hpp"
#include "middleware.hpp"

/**
 * @brief A vegan way to parse JSON Content in a response
 * @details TBC..
 *
 */
class Parsley : public server::Middleware {

public:

  virtual void process(
    server::Request_ptr req,
    server::Response_ptr res,
    server::Next next
    ) override
  {
    auto json = std::make_shared<Json>();
    json->members().insert({"name", "andreas"});

    req->set_attribute(json);

    if(req->has_attribute<Json>()) {
      auto name = req->get_attribute<Json>()->members()["name"];
      printf("<Parsley> Has JSON Attribute. name : %s\n", name.c_str());
    }
    else {
      printf("<Parsley> Has NOT JSON Attribute\n");
    }
    (*next)();
  }


};

#endif
