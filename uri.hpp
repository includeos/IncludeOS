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

#ifndef URI_HPP
#define URI_HPP

#include <gsl.h>

namespace uri {

  /**
   * Representation of RFC 3986 URI's.
   * Ref. https://tools.ietf.org/html/rfc3986
   **/
  class URI {

  public:

    /** String-like data type - at least as general as std::string */
    using String_t = gsl::span<char>;

    ///
    /// RFC-specified URI parts
    ///

    /** Get userinfo. E.g. 'username@'... */
    String_t userinfo();

    /** Get host. E.g. 'includeos.org', '10.0.0.42' etc. */
    String_t host();

    /** Get raw port number. In decimal character representation */
    String_t port_str();

    /** Get numeric port number.
     * @warning The RFC doesn't specify dimension. This funcion will truncate
     * any overflowing digits
     **/
    uint16_t port();

    /** Get the complete unparsed query string. */
    String_t query();

    /** Get the fragment part. E.g. "...#anchor1" */
    String_t fragment();

    ///
    /// Convenience
    ///

    /**
     * Get the URI-decoded value of a query-string key.
     *
     * E.g. for query() => "?name=Bjarne%20Stroustrup",
     * query("name") returns "Bjarne Stroustrup" */
    std::string query(String_t key);

  private:

    std::unordered_map<std::string,std::string> queries_;
    String_t uri_data_;

  } // class uri::URI

} // namespace uri

#endif
