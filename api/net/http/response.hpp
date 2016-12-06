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

#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include "message.hpp"
#include "status_codes.hpp"
#include "version.hpp"

namespace http {

///
/// This class is used to represent
/// an http response message
///
class Response : public Message {
public:
  ///
  /// Constructor to set up a response
  /// by providing information for the
  /// status line of the response message
  ///
  /// @param code    The status code
  /// @param version The version of the message
  ///
  explicit Response(const Version version = Version{1U, 1U}, const Code code = OK) noexcept;

  ///
  /// Constructor to construct a response
  /// message from the incoming character
  /// stream of data which is a {std::string}
  /// object
  ///
  /// @param response The character stream of data
  ///
  /// @param limit Capacity of how many fields can
  /// be added
  ///
  explicit Response(std::string response, const std::size_t limit = 25);

  ///
  /// Default copy constructor
  ///
  Response(const Response&) = default;

  ///
  /// Default move constructor
  ///
  Response(Response&&) = default;

  ///
  /// Default destructor
  ///
  ~Response() noexcept = default;

  ///
  /// Default copy assignment operator
  ///
  Response& operator = (const Response&) = default;

  ///
  /// Default move assignment operator
  ///
  Response& operator = (Response&&) = default;

  ///
  /// Get the status code of this
  /// message
  ///
  /// @return The status code of this
  /// message
  ///
  Code status_code() const noexcept;

  ///
  /// Change the status code of this
  /// message
  ///
  /// @param code The new status code to set
  /// on this message
  ///
  /// @return The object that invoked this method
  ///
  Response& set_status_code(const Code code) noexcept;

  ///
  /// Get the version of the response message
  ///
  /// @return The version of the response
  ///
  const Version version() const noexcept;

  ///
  /// Set the version of the response message
  ///
  /// @param version The version to set
  ///
  /// @return The object that invoked this
  /// method
  ///
  Response& set_version(const Version version) noexcept;

  ///
  /// Reset the response message as if it was now
  /// default constructed
  ///
  /// @return The object that invoked this method
  ///
  virtual Response& reset() noexcept override;

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
private:
  ///
  /// Class data members
  ///
  const std::string response_;

  ///
  /// Status-line parts
  ///
  Code    code_;
  Version version_;
}; //< class Response

/**--v----------- Helper Functions -----------v--**/

///
/// Add a set of headers to a response message
///
Response& operator << (Response& res, const Header_set& headers);

/**--^----------- Helper Functions -----------^--**/

/**--v----------- Implementation Details -----------v--**/

///////////////////////////////////////////////////////////////////////////////
template<typename = void>
inline Response_ptr make_response(std::string response) {
  return std::make_unique<Response>(std::move(response));
}

///////////////////////////////////////////////////////////////////////////////
inline std::ostream& operator << (std::ostream& output_device, const Response& res) {
  return output_device << res.to_string();
}

/**--^----------- Implementation Details -----------^--**/

} //< namespace http

#endif //< HTTP_RESPONSE_HPP
