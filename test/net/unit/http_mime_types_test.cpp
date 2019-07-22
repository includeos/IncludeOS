
#include <common.cxx>
#include <net/http/mime_types.hpp>

CASE("ext_to_mime_type() returns MIME type for specific extension")
{
  EXPECT(http::ext_to_mime_type("html") == "text/html");
  EXPECT(http::ext_to_mime_type("txt") == "text/plain");
  EXPECT(http::ext_to_mime_type("bin") == "application/octet-stream");
}

CASE("ext_to_mime_type() returns 'application/octet-stream' type for unknown extensions")
{
  EXPECT_NO_THROW(http::ext_to_mime_type(""));
  EXPECT(http::ext_to_mime_type("") == "application/octet-stream");
}
