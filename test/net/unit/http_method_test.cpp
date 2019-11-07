
#include <common.cxx>
#include <net/http/methods.hpp>

CASE("is_content_length_required() returns whether content length is required")
{
  const http::Method get(http::GET);
  const http::Method post(http::POST);
  EXPECT(http::method::is_content_length_required(get) == false);
  EXPECT(http::method::is_content_length_required(post) == true);
}

CASE("is_content_length_allowed() returns whether content length is allowed")
{
  const http::Method get(http::GET);
  const http::Method post(http::POST);
  const http::Method put(http::PUT);
  EXPECT(http::method::is_content_length_allowed(get) == false);
  EXPECT(http::method::is_content_length_allowed(post) == true);
  EXPECT(http::method::is_content_length_allowed(put) == true);
}

CASE("str() returns string representation of method")
{
  const http::Method m(http::GET);
  EXPECT(http::method::str(m) == "GET");
}

CASE("method() returns method as specified by string")
{
  const http::Method m1(http::method::code("GET"));
  EXPECT(http::method::is_content_length_required(m1) == false);
  const http::Method m2(http::method::code("TELEPORT"));
  EXPECT(http::method::str(m2) == "INVALID");
}
