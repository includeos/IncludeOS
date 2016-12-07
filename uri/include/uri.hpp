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

#pragma once
#ifndef URI_HPP
#define URI_HPP

#include <experimental/string_view>
#include <unordered_map>

namespace uri {

  /**
   * Representation of RFC 3986 URI's.
   * Ref. https://tools.ietf.org/html/rfc3986
   **/
  class URI {
  public:
    /*
     * Default constructor
     */
    explicit URI() = default;

    /*
     * Default copy and move constructors
     */
    URI(const URI&) = default;
    URI(URI&&) = default;

    /*
     * Default destructor
     */
    ~URI() noexcept = default;

    /*
     * Default assignment operators
     */
    URI& operator=(const URI&) = default;
    URI& operator=(URI&&) = default;

    /**
     * @brief Construct using a view into an existing string
     *
     * @param uri : A view into an existing string representing
     * a uri
     */
    explicit URI(const std::experimental::string_view uri);

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
    std::experimental::string_view scheme() const noexcept;

    /**
     * @brief Get userinfo.
     *
     * E.g. 'username@'...
     *
     * @return The user's information
     */
    std::experimental::string_view userinfo() const noexcept;

    /**
     * @brief Get host.
     *
     * E.g. 'includeos.org', '10.0.0.42' etc.
     *
     * @return The host's information
     */
    std::experimental::string_view host() const noexcept;

    /**
     * @brief Get the raw port number in decimal character representation.
     *
     * @return The raw port number as a {std::string}
     */
    std::experimental::string_view port_str() const noexcept;

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
    std::experimental::string_view path() const noexcept;

    /**
     * @brief Get the complete unparsed query string.
     *
     * @return The complete unparsed query string
     */
    std::experimental::string_view query() const noexcept;

    /**
     * @brief Get the fragment part.
     *
     * E.g. "...#anchor1"
     *
     * @return the fragment part
     */
    std::experimental::string_view fragment() const noexcept;

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
     * @brief Check to see if an object of this type is valid
     *
     * @return true if valid, false otherwise
     */
    bool is_valid() const noexcept;

    /**
     * @brief Convert an object of this type to a boolean value
     *
     * @see is_valid
     *
     * @return true if valid, false otherwise
     */
    operator bool() const noexcept;

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

    mutable int32_t port_ {-1};

    std::experimental::string_view scheme_;
    std::experimental::string_view userinfo_;
    std::experimental::string_view host_;
    std::experimental::string_view port_str_;
    std::experimental::string_view path_;
    std::experimental::string_view query_;
    std::experimental::string_view fragment_;

    std::unordered_map<std::string, std::string> queries_;

    /**
     * @brief Parse the given string view representing a uri
     * into its given parts according to RFC 3986
     */
    void parse();

    /**
     * @brief Load queries into the map
     */
    void load_queries();

  }; //< class URI

  /**
   * @brief Less-than operator to compare two {URI} objects
   *
   * @param lhs : {URI} object to compare
   * @param rhs : {URI} object to compare
   *
   * @return true if lhs is less-than rhs, false otherwise
   */
  bool operator < (const URI& lhs, const URI& rhs) noexcept;

  /**
   * @brief Operator to compare two {URI} objects
   * for equality
   *
   * @param lhs : {URI} object to compare
   * @param rhs : {URI} object to compare
   *
   * @return true if equal, false otherwise
   */
  bool operator == (const URI& lhs, const URI& rhs) noexcept;

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

} //< namespace uri

#endif //< URI_HPP
