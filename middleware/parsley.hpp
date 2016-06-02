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
    server::Response_ptr,
    server::Next next
    ) override
  {
    using namespace json;

    if(!has_json(req)) {
      printf("<Parsley> No JSON in header field.\n");
      (*next)();
      return;
    }
    printf("<Parsley> Found json header\n");

    // Request doesn't have JSON attribute
    if(!req->has_attribute<JsonDoc>()) {
      // create attribute
      auto json = std::make_shared<JsonDoc>();
      // access the document and parse the body
      json->doc().Parse(req->get_body().c_str());
      printf("<Parsley> Parsed JSON data.\n");
      // add the json to the request
      req->set_attribute(json);
    }

    (*next)();
  }

private:
  bool has_json(server::Request_ptr req) const {
    auto c_type = http::header_fields::Entity::Content_Type;
    if(!req->has_header(c_type))
      return false;
    return (req->header_value(c_type).find("application/json") != std::string::npos);
  }

};

#endif
