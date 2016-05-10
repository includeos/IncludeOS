#include "request.hpp"

using namespace server;

Request::Request(buffer_t buf, size_t n)
  : Parent(std::string{(char*)buf.get(), n})
{

}
