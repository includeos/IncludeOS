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

#include "logger.hpp"
#include <sstream>
#include <common>

Logger::Logger(Log& log, Log::index_type pos)
: log_(log), pos_{&log_, pos}
{
}

void Logger::log(const std::string& str) {

  if(UNLIKELY( str.empty() ))
    return;

  if(UNLIKELY( str.size() + 1 >= (unsigned)log_.size() ))
  {
    // start at the beginning
    pos_ = {&log_, 0};
    // calculate offset (with padding)
    auto offset = str.size() + 1 - log_.size();
    // copy later part into position
    std::copy(str.begin() + offset, str.end(), pos_);
    // increment position to know where to put padding
    pos_ += str.size() - offset;
    *pos_ = '\0';
    // return to the beginning
    ++pos_;
    return;
  }

  std::copy(str.begin(), str.end(), pos_);
  pos_ += str.size();

  // add null terminate padding
  auto it = pos_;
  while(*it != '\0') {
    *it = '\0';
    ++it;
  }
  ++pos_;
}

std::vector<std::string> Logger::entries() const {
  std::vector<std::string> results;

  auto head = pos_;

  do {
    // adjust head
    while(*head == '\0') {
      ++head;
      // return results if we went all the way around
      if(head == pos_)
        return results;
    }

    std::vector<char> vec;

    while(*head != '\0') {
      vec.emplace_back(*head);
      ++head;
    }

    results.emplace_back(vec.begin(), vec.end());

  } while(true);

  return results;
}

std::vector<std::string> Logger::entries(size_t n) const {
  std::vector<std::string> results;

  results.reserve(n);

  auto head = pos_;

  while(n != 0) {
    // adjust head
    while(*head == '\0') {
      --head;
      // return results if we went all the way around
      if(head == pos_)
        return results;
    }
    // temporary string
    std::vector<char> vec;
    do {
      // build the string from back to front
      vec.emplace_back(*head);
      --head;
    } while(*head != '\0');

    // emplace the string in reverese
    results.emplace_back(vec.rbegin(), vec.rend());
    // decrement remaining entries
    n--;
  }

  // reverese all the results so it became oldest first
  std::reverse(results.begin(), results.end());

  return results;
}

void Logger::flush() {
  std::fill(log_.begin(), log_.end(), 0);
  pos_ = {&log_, 0};
}
