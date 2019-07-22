
#ifndef HTTP_MIME_TYPES_HPP
#define HTTP_MIME_TYPES_HPP

#include "../../util/detail/string_view"

namespace http {

///
/// Get the mime type for the specified extension
///
/// @param extension The specified extension to get associated mime type
///
/// @return The associated mime type for the specified extension
///
util::sview ext_to_mime_type(util::csview extension) noexcept;

} //< namespace http

#endif //< HTTP_MIME_TYPES_HPP
