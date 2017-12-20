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

#include <net/http/header.hpp>

namespace http {

///////////////////////////////////////////////////////////////////////////////
Header::Header() {
  fields_.reserve(25);
}

///////////////////////////////////////////////////////////////////////////////
Header::Header(const std::size_t limit) {
  fields_.reserve(limit);
}

///////////////////////////////////////////////////////////////////////////////
bool Header::add_field(std::string field, std::string value) {
  if (field.empty()) return false;
  //-----------------------------------
  if (size() < fields_.capacity()) {
    fields_.emplace_back(std::move(field), std::move(value));
    return true;
  }
  //-----------------------------------
  return false;
}

///////////////////////////////////////////////////////////////////////////////
bool Header::set_field(std::string field, std::string value) {
  if (field.empty() || value.empty()) return false;
  //-----------------------------------
  const auto target = find(field);
  //-----------------------------------
  if (target not_eq fields_.cend()) {
    const_cast<std::string&>(target->second) = std::move(value);
    return true;
  }
  //-----------------------------------
  else return add_field(std::move(field), std::move(value));
}

///////////////////////////////////////////////////////////////////////////////
bool Header::has_field(util::csview field) const noexcept {
  return find(field) not_eq fields_.cend();
}

///////////////////////////////////////////////////////////////////////////////
util::sview Header::value(util::csview field) const noexcept {
  if (field.empty()) return field;
  const auto it = find(field);
  return (it not_eq fields_.cend()) ? util::csview{it->second} : util::sview();
}

///////////////////////////////////////////////////////////////////////////////
bool Header::is_empty() const noexcept {
  return fields_.empty();
}

///////////////////////////////////////////////////////////////////////////////
std::size_t Header::size() const noexcept {
  return fields_.size();
}

///////////////////////////////////////////////////////////////////////////////
void Header::erase(util::csview field) noexcept {
  Const_iterator target;
  while ((target = find(field)) not_eq fields_.cend()) fields_.erase(target);
}

///////////////////////////////////////////////////////////////////////////////
void Header::clear() noexcept {
  fields_.clear();
}

///////////////////////////////////////////////////////////////////////////////
size_t Header::content_length() const noexcept {
  try {
    const auto cl = value(header::Content_Length);
    if(cl.empty()) return 0;
    return std::stoul(std::string{cl.data(), cl.length()});
  } catch(...) {
    return 0;
  }
}

///////////////////////////////////////////////////////////////////////////////
bool Header::set_content_length(const size_t len) {
  return set_field(header::Content_Length, std::to_string(len));
}

///////////////////////////////////////////////////////////////////////////////
Header::Const_iterator Header::find(util::csview field) const noexcept {
  if (field.empty()) return fields_.cend();
  //-----------------------------------
  return
    std::find_if(fields_.cbegin(), fields_.cend(), [&field](const auto _) {
      return std::equal(_.first.cbegin(), _.first.cend(), field.cbegin(), field.cend(),
        [](const auto a, const auto b) { return std::tolower(a) == std::tolower(b); });
    });
}

} //< namespace http
