
#include <common.cxx>
#include <uri>
#include <strings.h> // strcasecmp

CASE("uri::encode() encodes a string")
{
  std::string encoded {uri::encode("The C++ Programming Language (4th Edition)")};
  static const char* expected = "The%20C%2B%2B%20Programming%20Language%20%284th%20Edition%29";
  EXPECT(strcasecmp(encoded.c_str(), expected) == 0);
}

CASE("uri::decode() decodes an URI-encoded string")
{
  std::string decoded {uri::decode("The%20C%2B%2B%20Programming%20Language%20%284th%20Edition%29")};
  EXPECT(decoded == "The C++ Programming Language (4th Edition)");
}

CASE("uri::decode() throws on invalid input when URI_THROW_ON_ERROR is defined")
{
  EXPECT_THROWS_AS(std::string s = uri::decode("%2x%zz"), std::runtime_error);
}
