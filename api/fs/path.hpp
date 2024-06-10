// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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
#ifndef FS_PATH_HPP
#define FS_PATH_HPP

#include <deque>
#include <string>
#include <stdexcept>
#include <expects>

namespace fs {

  class Path {
  public:

    struct Err_invalid_path : public std::runtime_error {
      using runtime_error::runtime_error;
    };

    //! constructs Path to the current directory
    Path();

    //! constructs Path to @path
    Path(const std::string& path);

    Path(std::initializer_list<std::string>);

    size_t size() const noexcept
    { return stk.size(); }

    const std::string& operator [] (const int i) const
    { return stk.at(i); }

    int state() const noexcept
    { return state_; }

    Path& operator = (const std::string& p) {
      stk.clear();
      parse_add(p);
      return *this;
    }

    Path& operator += (const std::string& p) {
      this->state_ = parse_add(p);
      return *this;
    }

    Path operator + (const std::string& p) const {
      Path np  = Path(*this);
      np.state_ = np.parse_add(p);
      return np;
    }

    bool operator == (const Path& p) const {
      if (stk.size() not_eq p.stk.size()) return false;
      return this->to_string() == p.to_string();
    }

    bool operator != (const Path& p) const
    { return not this->operator == (p); }

    bool operator == (const std::string& p) const
    { return *this == Path(p); }

    bool empty() const noexcept
    { return stk.empty(); }

    auto begin() const
    { return stk.begin(); }

    auto end() const
    { return stk.end(); }

    std::string front() const
    {
      Expects(not empty()); // front() on empty container is undefined behaviour
      return stk.front();
    }

    std::string back() const
    {
      Expects(not empty()); // back() on empty container is undefined behaviour
      return stk.back();
    }

    Path& pop_front()
    {
      Expects(not empty()); // pop_front() from empty container is undefined
      stk.pop_front(); return *this;
    }

    Path& pop_back()
    {
      Expects(not empty()); // pop_back() from empty container is undefined
      stk.pop_back(); return *this;
    }

    Path& up()
    { if (not stk.empty()) stk.pop_back(); return *this; }

    std::string to_string() const;

  private:
    /** Parse string and add parts to this path **/
    int  parse_add(const std::string& path);
    void name_added(const std::string& name);
    std::string real_path() const;

    int state_;
    std::deque<std::string> stk;
  }; //< class Path

} //< namespace fs

#endif //< FS_PATH_HPP
