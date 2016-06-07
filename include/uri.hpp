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
#include <unordered_map>

namespace uri {

  /**
   * Representation of RFC 3986 URI's.
   * Ref. https://tools.ietf.org/html/rfc3986
   **/
  class URI {
  private:
    /** Non-owning pointer-size type */
    struct Span_t {
      size_t begin;
      size_t end;

      Span_t(const size_t b = 0U, const size_t e = 0U) noexcept
        : begin{b}
        , end{e}
      {}
    }; //< struct Span_t

  public:
    ///
    /// RFC-specified URI parts
    ///

    /** Get userinfo. E.g. 'username@'... */
    const std::string userinfo() const;

    /** Get host. E.g. 'includeos.org', '10.0.0.42' etc. */
    const std::string host() const;

    /** Get raw port number. In decimal character representation */
    const std::string port_str() const;

    /** Get numeric port number.
     * @warning The RFC doesn't specify dimension. This funcion will truncate
     * any overflowing digits
     **/
    uint16_t port() const;

    /** Get the path. E.g. /pictures/logo.png  */
    std::string path() const;

    /** Get the complete unparsed query string. */
    const std::string query() const;

    /** Get the fragment part. E.g. "...#anchor1" */
    const std::string fragment() const;

    ///
    /// Construct / Destruct
    ///

    URI() = default;
    URI(URI&) = default;
    URI(URI&&) = default;
    ~URI() = default;
    URI& operator=(const URI&) = default;
    URI& operator=(URI&&) = default;

    // We might do a span-based constructor later.
    //URI(gsl::span<const char>);

    /** Construct using a string */
    URI(const std::string&&);
    URI(const char*);

    ///
    /// Convenience
    ///

    /**
     * Get the URI-decoded value of a query-string key.
     *
     * E.g. for query() => "?name=Bjarne%20Stroustrup",
     * query("name") returns "Bjarne Stroustrup" */
    std::string query(std::string key);

    /** String representation **/
    std::string to_string() const;

  private:

    std::unordered_map<std::string,std::string> queries_;
    Span_t uri_data_;
    Span_t userinfo_;
    Span_t host_;
    Span_t port_str_;
    uint16_t port_ = 0;
    Span_t path_;
    Span_t query_;
    Span_t fragment_;

    static const Span_t zero_span_;

    // A copy of the data, if the string-based constructor was used
    std::string uri_str_;

    /**
     * @brief Parse the given string representing a uri
     * into its given parts according to RFC 3986
     *
     * @param uri : The string representing a uri
     */
    void parse(const std::string& uri);

  }; // class uri::URI


  std::ostream& operator<< (std::ostream&, const URI&);

} // namespace uri

#endif
