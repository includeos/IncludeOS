// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016 Oslo and Akershus University College of Applied Sciences
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

#ifndef AUTHORS_HPP
#define AUTHORS_HPP

#include <iomanip>
#include <util/collections.hpp>

///
/// This class represent an author of some artifact within
/// a repository
///
class Author {
public:
  ///
  /// Constructor
  ///
  /// @param name
  ///   Name of the author
  ///
  /// @param email
  ///   Email address of the author
  ///
  /// @param country
  ///   Country the author is from
  ///
  Author(const std::string& name, const std::string& email, const std::string& country);

  ///
  /// Default destructor
  ///
  ~Author() = default;

  ///
  /// Get the name of the author
  ///
  /// @return The name of the author
  ///
  const std::string& name() const noexcept;

  ///
  /// Get the email address of the author
  ///
  /// @return The email address of the author
  ///
  const std::string& email() const noexcept;

  ///
  /// Get the country the author is from
  ///
  /// @return The country the author is from
  ///
  const std::string& country() const noexcept;

  ///
  /// Get a string representation of this class
  ///
  /// @return A string representation
  ///
  std::string to_string() const;

  ///
  /// Operator to transform this class into string form
  ///
  operator std::string () const;

  ///
  /// Operator to check less than relationship for objects
  /// of this type
  ///
  bool operator < (const Author& author) const noexcept;

  ///
  /// Operator to check equality relationship for objects
  /// of this type
  ///
  bool operator == (const Author& author) const noexcept;

private:
  std::string name_;
  std::string email_;
  std::string country_;
}; //< class Author

///
/// This class represent a list of authors within a repository
///
class Author_list {
public:
  using value_type = Author;

  ///
  /// Default constructor
  ///
  explicit Author_list() noexcept = default;

  ///
  /// Default destructor
  ///
  ~Author_list() = default;

  ///
  /// Constructor taking a list of authors
  ///
  /// @param author_list
  ///   A list of authors
  ///
  explicit Author_list(const std::initializer_list<Author>& author_list);

  ///
  /// Add an author to the list of authors
  ///
  /// @param name
  ///   The name of the author
  ///
  /// @param email
  ///   The email address of the author
  ///
  /// @param country
  ///   The country where the author is from
  ///
  /// @return The object that invoked this method
  ///
  Author_list& add(std::string name, std::string email, std::string country);

  ///
  /// Add a list of authors to the list
  ///
  /// @param author_list
  ///   A list of authors to add to the list
  ///
  /// @return The object that invoked this method
  ///
  Author_list& add(const std::initializer_list<Author>& author_list);

  ///
  /// Get an iterator to the beginning of the list
  ///
  /// @return An iterator to the beginning of the list
  ///
  std::vector<Author>::iterator begin() noexcept;

  ///
  /// Get an iterator to the end of the list
  ///
  /// @return An iterator to the end of the list
  ///
  std::vector<Author>::iterator end() noexcept;

  ///
  /// Get a const_iterator to the beginning of the list
  ///
  /// @return A const_iterator to the beginning of the list
  ///
  std::vector<Author>::const_iterator cbegin() const noexcept;

  ///
  /// Get a const_iterator to the beginning of the list
  ///
  /// @return A const_iterator to the beginning of the list
  ///
  std::vector<Author>::const_iterator cend() const noexcept;

  ///
  /// Get a string representation of this class
  ///
  /// @param sorted
  ///   Whether to sort the list
  ///
  /// @param header
  ///   Whether to add a header to the list
  ///
  /// @return A string representation
  ///
  std::string to_string(const bool sorted = true, const bool header = false);

  ///
  /// Operator to transform this class into string form
  ///
  operator std::string ();
private:
  std::vector<Author> authors_;
}; //< class Author_list

/**--v----------- Implementation Details -----------v--**/

inline Author::Author(const std::string& name, const std::string& email, const std::string& country)
  : name_{name}
  , email_{email}
  , country_{country}
{}

inline const std::string& Author::name() const noexcept {
  return name_;
}

inline const std::string& Author::email() const noexcept {
  return email_;
}

inline const std::string& Author::country() const noexcept {
  return country_;
}

inline std::string Author::to_string() const {
  std::ostringstream author;
  author << name_ << ' ' << email_ << ' ' << country_;
  return author.str();
}

inline Author::operator std::string () const {
  return to_string();
}

inline bool Author::operator < (const Author& author) const noexcept {
  return name() < author.name();
}

inline bool Author::operator == (const Author& author) const noexcept {
  return name() == author.name();
}

inline std::ostream& operator << (std::ostream& output_device, const Author& author) {
  return output_device << author.to_string();
}

inline Author_list::Author_list(const std::initializer_list<Author>& author_list)
  : authors_{author_list}
{}

inline Author_list& Author_list::add(std::string name, std::string email, std::string country) {
  authors_.emplace(authors_.cend(), std::move(name), std::move(email), std::move(country));
  return *this;
}

inline Author_list& Author_list::add(const std::initializer_list<Author>& author_list) {
  authors_.insert(authors_.cend(), author_list);
  return *this;
}

inline std::vector<Author>::iterator Author_list::begin() noexcept {
  return authors_.begin();
}

inline std::vector<Author>::iterator Author_list::end() noexcept {
  return authors_.end();
}

inline std::vector<Author>::const_iterator Author_list::cbegin() const noexcept {
  return authors_.cbegin();
}

inline std::vector<Author>::const_iterator Author_list::cend() const noexcept {
  return authors_.cend();
}

inline std::string Author_list::to_string(const bool sorted, const bool header) {
  std::ostringstream output_device;

  const auto& vauthors = (not sorted) ? authors_ : (authors_ | collections::stable_sort);

  const auto max_name    = collections::map(vauthors, [](const Author& _)
  { return _.name().length(); })    | collections::max_element;
  const auto max_email   = collections::map(vauthors, [](const Author& _)
  { return _.email().length(); })   | collections::max_element;
  const auto max_country = collections::map(vauthors, [](const Author& _)
  { return _.country().length(); }) | collections::max_element;

  if (header) {
    std::cout << std::setw(max_name)    << std::left << "Name"    << ' '
              << std::setw(max_email)   << std::left << "E-mail"  << ' '
              << std::setw(max_country) << std::left << "Country" << "\n\n";
  }

  for (const auto& author : vauthors) {
    output_device << std::setw(max_name)    << std::left << author.name()    << ' '
                  << std::setw(max_email)   << std::left << author.email()   << ' '
                  << std::setw(max_country) << std::left << author.country() << '\n';
  }

  return output_device.str();
}

inline Author_list::operator std::string () {
  return to_string();
}

inline std::ostream& operator << (std::ostream& output_device, Author_list& authors) {
  return output_device << authors.to_string(true, true);
}

/**--^----------- Implementation Details -----------^--**/

#endif //< AUTHORS_HPP
