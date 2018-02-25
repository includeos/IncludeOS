#include <cmath> // rand()
#include <sstream>
#include <net/http/request.hpp>
#include <net/http/response.hpp>

using namespace std::chrono;

std::string HTML_RESPONSE()
{
  const int color = rand();

  // Generate some HTML
  std::stringstream stream;
  stream << "<!DOCTYPE html><html><head>"
         << "<link href='https://fonts.googleapis.com/css?family=Ubuntu:500,300'"
         << " rel='stylesheet' type='text/css'>"
         << "<title>IncludeOS Demo Service</title></head><body>"
         << "<h1 style='color: #" << std::hex << ((color >> 8) | 0x020202)
         << "; font-family: \"Arial\", sans-serif'>"
         << "Include<span style='font-weight: lighter'>OS</span></h1>"
         << "<h2>The C++ Unikernel</h2>"
         << "<p>You have successfully booted an IncludeOS TCP service with simple https. "
         << "For a more sophisticated example, take a look at "
         << "<a href='https://github.com/hioa-cs/IncludeOS/tree/master/examples/acorn'>Acorn</a>.</p>"
         << "<footer><hr/>&copy; 2017 IncludeOS </footer></body></html>";

  return stream.str();
}

http::Response_ptr handle_request(const http::Request& req)
{
  auto res = http::make_response();
  auto& header = res->header();

  header.set_field(http::header::Server, "IncludeOS/0.12");

  // GET /
  if(req.method() == http::GET && req.uri().to_string() == "/")
  {
    // add HTML response
    res->add_body(HTML_RESPONSE());

    // set Content type and length
    header.set_field(http::header::Content_Type, "text/html; charset=UTF-8");
    header.set_field(http::header::Content_Length, std::to_string(res->body().size()));
  }
  else
  {
    // Generate 404 response
    res->set_status_code(http::Not_Found);
  }

  header.set_field(http::header::Connection, "close");
  return res;
}
