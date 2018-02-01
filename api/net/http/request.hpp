// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <stdexcept>

#include "message.hpp"
#include "methods.hpp"
#include "version.hpp"

namespace http {

///
/// This class is used to represent an error that occurred
/// from within the operations of class Request
///
class Request_error : public std::runtime_error {
public:
  using runtime_error::runtime_error;
}; //< class Request_error

///
/// This class is used to represent
/// an http request message
///
class Request : public Message {
public:
  ///
  /// Default constructor
  ///
  explicit Request() = default;

  ///
  /// Constructor to construct a request
  /// message from the incoming character
  /// stream of data which is a {std::string}
  /// object
  ///
  /// @param request The character stream of data
  ///
  /// @param limit Capacity of how many fields can
  /// be added
  ///
  /// @param parse Whether to perform parsing on the the data specified in {request}
  ///
  explicit Request(std::string request, const std::size_t limit = 25, const bool parse = true);

  ///
  /// Default copy constructor
  ///
  Request(Request&) = default;

  ///
  /// Default move constructor
  ///
  Request(Request&&) = default;

  ///
  /// Default destructor
  ///
  ~Request() noexcept = default;

  ///
  /// Default copy assignment operator
  ///
  Request& operator = (Request&) = default;

  ///
  /// Default move assignment operator
  ///
  Request& operator = (Request&&) = default;

  ///
  /// Parse the information supplied to the Request object
  ///
  /// @return The object that invoked this method
  ///
  Request& parse();

  ///
  /// Get the method of the request message
  ///
  /// @return The method of the request
  ///
  Method method() const noexcept;

  ///
  /// Set the method of the request message
  ///
  /// @param method The method to set
  ///
  /// @return The object that invoked this
  /// method
  ///
  Request& set_method(const Method method);

  ///
  /// Get a reference to the URI object of the
  /// request message
  ///
  /// @return The URI of the request
  ///
  const URI& uri() const noexcept;

  ///
  /// Set the URI of the request message
  ///
  /// @param uri The URI to set
  ///
  /// @return The object that invoked this
  /// method
  ///
  Request& set_uri(const URI& uri);

  ///
  /// Get the version of the request message
  ///
  /// @return The version of the request
  ///
  const Version& version() const noexcept;

  ///
  /// Set the version of the request message
  ///
  /// @param version The version to set
  ///
  /// @return The object that invoked this
  /// method
  ///
  Request& set_version(const Version& version) noexcept;

  ///
  /// Get the value associated with the name
  /// from a query string
  ///
  /// @param name The name to find the associated value
  ///
  /// @return The associated value if name was found,
  /// an empty string otherwise
  ///
  template<typename = void>
  util::sview query_value(util::csview name) noexcept;

  ///
  /// Get the value associated with the name
  /// field from the message body in a post request
  ///
  /// @param name The name to find the associated
  /// value
  ///
  /// @return The associated value if name was found,
  /// an empty string otherwise
  ///
  template<typename = void>
  util::sview post_value(util::csview name) const noexcept;

  ///
  /// Reset the request message as if it was now
  /// default constructed
  ///
  /// @return The object that invoked this method
  ///
  virtual Request& reset() noexcept override;

  ///
  /// Get a string representation of this
  /// class
  ///
  /// @return A string representation
  ///
  virtual std::string to_string() const override;

  ///
  /// Operator to transform this class
  /// into string form
  ///
  operator std::string () const;

  ///
  /// Stream a chunk of new data into the request for parsing
  ///
  /// @param chunk A new set of data to append to request for parsing
  ///
  /// @return The object that invoked this method
  ///
  Request& operator << (const std::string& chunk);
private:
  ///
  /// Class data members
  ///
  std::string request_;

  ///
  /// Request-line parts
  ///
  Method  method_{GET};
  URI     uri_{"/"};
  Version version_{1U, 1U};

  ///
  /// Reset the object for reparsing the accumulated request
  /// information
  ///
  /// @return The object that invoked this method
  ///
  Request& soft_reset() noexcept;
}; //< class Request

/**--v----------- Implementation Details -----------v--**/

///////////////////////////////////////////////////////////////////////////////
template<typename>
inline util::sview Request::query_value(util::csview name) noexcept {
  return uri_.query(std::string(name));
}

///////////////////////////////////////////////////////////////////////////////
template<typename>
inline util::sview Request::post_value(util::csview name) const noexcept {
  if ((method() not_eq POST) or name.empty() or body().empty()) {
    return {};
  }
  //---------------------------------
  const auto target = body().find(name);
  //---------------------------------
  if (target == util::sview::npos) return {};
  //---------------------------------
  auto focal_point = body().substr(target);
  //---------------------------------
  focal_point = focal_point.substr(0, focal_point.find_first_of('&'));
  //---------------------------------
  const auto lock_and_load = focal_point.find('=');
  //---------------------------------
  if (lock_and_load == util::sview::npos) return {};
  //---------------------------------
  return focal_point.substr(lock_and_load + 1);
}

///////////////////////////////////////////////////////////////////////////////
template<typename = void>
inline Request_ptr make_request() {
  return std::make_unique<Request>();
}

///////////////////////////////////////////////////////////////////////////////
template<typename = void>
inline Request_ptr make_request(std::string request) {
  return std::make_unique<Request>(std::move(request));
}

///////////////////////////////////////////////////////////////////////////////
template<typename Char, typename Char_traits>
inline std::basic_ostream<Char, Char_traits>& operator<<(std::basic_ostream<Char, Char_traits>& output_device, const Request& req) {
  return output_device << req.to_string();
}

/**--^----------- Implementation Details -----------^--**/

} //< namespace http

#endif //< HTTP_REQUEST_HPP
