#include <uri>
#include <lest.hpp>

using namespace std::string_literals;

const lest::test specification[] = {
    CASE("Bogus URI is invalid") {
      uri::URI uri {"???!?!"s};
      EXPECT(uri.is_valid() == false);
    },

    CASE("scheme() returns the URI's scheme") {
      uri::URI uri {"http://www.vg.no"s};
      EXPECT(uri.scheme() == "http"s);
    },

    CASE("userinfo() returns the URI's user information") {
        uri::URI uri {"http://Aladdin:OpenSesame@www.vg.no"s};
        EXPECT(uri.userinfo() == "Aladdin:OpenSesame@"s);
    },

    CASE("host() returns the URI's host") {
      uri::URI uri {"http://www.vg.no/"s}; //?
      EXPECT(uri.host() == "www.vg.no"s);
    },

    CASE("host() returns the URI's host (no path)") {
      uri::URI uri {"http://www.vg.no"s};
      EXPECT(uri.host() == "www.vg.no"s);
    },

    CASE("port() returns the URI's port") {
      uri::URI uri {"http://www.vg.no:80/"s};
      EXPECT(uri.port() == 80);
    },

    CASE("port() returns the URI's port (no path)") {
      uri::URI uri {"http://www.vg.no:80"s};
      EXPECT(uri.port() == 80);
    },

    CASE("Out-of-range ports are detected as invalid") {
      uri::URI uri {"http://www.vg.no:65539"s};
      EXPECT(uri.is_valid() == false);
    },

    CASE("path() returns the URI's port") {
      uri::URI uri {"http://www.digi.no/artikler/nytt-norsk-operativsystem-vekker-oppsikt/319971"s};
      EXPECT(uri.path() == "/artikler/nytt-norsk-operativsystem-vekker-oppsikt/319971");
    },

    CASE("query() returns unparsed query string") {
        uri::URI uri {"http://www.vg.no?fname=patrick&lname=bateman"s};
        EXPECT(uri.query() == "?fname=patrick&lname=bateman"s);
    },

    CASE("query(\"param\") returns value of query parameter named \"param\"") {
      uri::URI uri {"http://www.vg.no?fname=patrick&lname=bateman"s};
      EXPECT(uri.query("fname") == "patrick"s);
    },

    CASE("query(\"param\") returns missing query parameters as empty string") {
      uri::URI uri {"http://www.vg.no?fname=patrick&lname=bateman"s};
      EXPECT(uri.query("phone") == ""s);
    },

    CASE("fragment() returns fragment part") {
      uri::URI uri {"https://github.com/includeos/acorn#take-it-for-a-spin"s};
      EXPECT(uri.fragment() == "#take-it-for-a-spin"s);
    },

    CASE("Invalid port does not crash the parser") {
      uri::URI uri {"http://www.vg.no:80999999999999999999999999999"s};
      int port {0};
      EXPECT_NO_THROW(port = uri.port());
    }
};

int main(int argc, char * argv[]) {
    return lest::run(specification, argc, argv);
}
