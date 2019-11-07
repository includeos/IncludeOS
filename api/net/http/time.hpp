
#ifndef HTTP_TIME_HPP
#define HTTP_TIME_HPP

#include <ctime>
#include <iomanip>
#include <sstream>

#include "../../util/detail/string_view"

namespace http {
namespace time {

///
/// Get the time in {Internet Standard Format} from
/// a {time_t} object
///
/// @param time_ The {time_t} object to get the time from
///
/// @return The time in {Internet Standard Format} as a std::string
///
/// @note Returns an empty string if an error occurred
///
std::string from_time_t(const std::time_t time_);

///
/// Get a {time_t} object from a {std::string} representing
/// timestamps specified in RFC 2616 §3.3
///
/// @param time_ The {std::string} representing the timestamp
///
/// @return A {time_t} object from {time_}
///
/// @note Returns a default initialized {time_t} object if an error occurred
///
std::time_t to_time_t(util::csview time_);

///
/// Get the current time in {Internet Standard Format}
///
/// @return The current time as a std::string
///
/// @note Returns an empty string if an error occurred
///
std::string now();

} //< namespace time
} //< namespace http

#endif //< HTTP_TIME_HPP
