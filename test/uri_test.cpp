#include <uri>
#include <lest.hpp>

using namespace std::string_literals;

const lest::test specification[] = {
    CASE("Bogus URI is invalid") {
      uri::URI uri {"?!?!"s};
      EXPECT(uri.is_valid() == false);
    },

    CASE("port() returns the URI's port") {
      uri::URI uri {"http://www.vg.no:80"s};
      EXPECT(uri.port() == 80);
    },

    CASE("host() returns the URI's host") {
      uri::URI uri {"http://www.vg.no"s};
      EXPECT(uri.host() == "www.vg.no"s);
    },

    CASE("Out-of-range ports are detected as invalid") {
      uri::URI uri {"http://www.vg.no:65539"s};
      EXPECT(uri.is_valid() == false);
    },

    CASE("Invalid port does not crash the parser") {
      uri::URI uri {"http://www.vg.no:80999999999999999999999999999"s};
      int port {0};
      EXPECT_NO_THROW(port = uri.port());
    }
};

int main(int argc, char * argv[]) {
    return lest::run( specification, argc, argv );
}
