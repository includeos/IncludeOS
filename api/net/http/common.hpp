
#ifndef HTTP_COMMON_HPP
#define HTTP_COMMON_HPP

#include <delegate>
#include <memory>
#include <uri>
#include <utility>
#include <vector>

#include "../../util/detail/string_view"

namespace http {

using URI = uri::URI;

using Header_set = std::vector<std::pair<std::string, std::string>>;

class Request;
using Request_ptr = std::unique_ptr<Request>;

class Response;
using Response_ptr = std::unique_ptr<Response>;

class Server;
using Server_ptr = std::unique_ptr<Server>;

} //< namespace http

#endif //< HTTP_COMMON_HPP
