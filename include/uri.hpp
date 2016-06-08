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
    /*
     * Default constructor
     */
    URI() = default;

    /*
     * Default copy and move constructors
     */
    URI(URI&) = default;
    URI(URI&&) = default;

    /*
     * Default destructor
     */
    ~URI() = default;

    /*
     * Default assignment operators
     */
    URI& operator=(const URI&) = default;
    URI& operator=(URI&&) = default;

    //
    // We might do a span-based constructor later.
    // URI(gsl::span<const char>);
    //

    /**
     * @brief Construct using a C-String
     *
     * @param uri : A C-String representing a uri
     */
    URI(const char* uri);

    /**
     * @brief Construct using a string
     *
     * @param uri : A string representing a uri
     */
    URI(const std::string& uri);

    ///////////////////////////////////////////////
    //----------RFC-specified URI parts----------//
    ///////////////////////////////////////////////

    /**
     * @brief Get scheme.
     *
     * E.g. 'http', 'file', 'ftp' etc.
     *
     * @return The scheme
     */
    const std::string& scheme() const;

    /**
     * @brief Get userinfo.
     *
     * E.g. 'username@'...
     *
     * @return The user's information
     */
    const std::string& userinfo() const;

    /**
     * @brief Get host.
     *
     * E.g. 'includeos.org', '10.0.0.42' etc.
     *
     * @return The host's information
     */
    const std::string& host() const;

    /**
     * @brief Get the raw port number in decimal character representation.
     *
     * @return The raw port number as a string
     */
    const std::string& port_str() const;

    /**
     * @brief Get numeric port number.
     *
     * @warning The RFC doesn't specify dimension. This funcion will truncate
     * any overflowing digits.
     *
     * @return The numeric port number as a 16-bit number
     */
    uint16_t port() const noexcept;

    /**
     * @brief Get the path.
     *
     * E.g. /pictures/logo.png 
     *
     * @return The path information
     */
    const std::string& path() const;

    /**
     * @brief Get the complete unparsed query string.
     *
     * @return The complete unparsed query string
     */
    const std::string& query() const;

    /**
     * @brief Get the fragment part.
     *
     * E.g. "...#anchor1"
     *
     * @return the fragment part
     */
    const std::string& fragment() const;

    /**
     * @brief Get the URI-decoded value of a query-string key.
     *
     * @param key : The key to find the associated value
     *
     * @return The key's associated value
     *
     * @example For the query: "?name=Bjarne%20Stroustrup",
     * query("name") returns "Bjarne Stroustrup"
     */
    const std::string& query(const std::string& key);

    /**
     * @brief Get a string representation of this
     * class
     *
     * @return - A string representation
     */
    std::string to_string() const;

    /**
     * @brief Operator to transform this class
     * into string form
     */
    operator std::string () const;

  private:
    /*
     * A copy of the data representing a uri
     */
    std::string uri_str_;

    mutable uint16_t port_;

    Span_t scheme_;
    Span_t userinfo_;
    Span_t host_;
    Span_t port_str_;
    Span_t path_;
    Span_t query_;
    Span_t fragment_;

    static const Span_t zero_span_;

    std::unordered_map<std::string,std::string> queries_;

    /**
     * @brief Parse the given string representing a uri
     * into its given parts according to RFC 3986
     *
     * @param uri : The string representing a uri
     */
    void parse(const std::string& uri);

    /**
     * @brief Load queries into the map
     */
    void load_queries();

  }; // class uri::URI

  /**
   * @brief Operator to stream the contents of a {URI}
   * to the specified output stream device
   *
   * @param output_device : The output stream device
   * @param uri : The {URI} to send to the output stream
   *
   * @return A reference to the specified output stream device
   */
  std::ostream& operator<< (std::ostream& output_device, const URI& uri);

} // namespace uri

#endif //< URI_HPP
