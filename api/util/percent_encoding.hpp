

///
/// URI encode/decode (percent encoding) module
///
/// Implemented for IncludeOS from RFC 3986
/// https://tools.ietf.org/html/rfc3986#section-2.1
///
/// @note Encoding reserved chars:
/// The RFC lets the application decide which reserved characters to
/// encode and which can be used as data. This implementation
/// currently encodes all of them.
///
/// @note Decoding errors:
/// The RFC only defines what an URI-encoded string is, not what to do
/// if you try to decode a non-encoded (or erroneously encoded)
/// string. Doing that is therefore considered undefined. By default
/// this implementation will partially decode as much as possible and
/// return that. To throw on error instead, define URI_THROW_ON_ERROR.
///

#pragma once
#ifndef PERCENT_ENCODING_HPP
#define PERCENT_ENCODING_HPP

#include "detail/string_view"
#include <string>

namespace uri {

///
/// Encode (percent-encode) a view of data representing a uri
///
std::string encode(util::csview input);

///
/// Decode (percent-decode) a view of data representing a uri
///
std::string decode(util::csview input);


#ifdef URI_THROW_ON_ERROR
#include <stdexcept>

struct Decode_error : public std::runtime_error {
  using runtime_error::runtime_error;
};

struct Encode_error : public std::runtime_error {
  using runtime_error::runtime_error;
};

struct Hex_error : public Decode_error {
  using Decode_error::Decode_error;
};

#endif //< URI_THROW_ON_ERROR

} //< namespace uri

#endif //< PERCENT_ENCODING_HPP
