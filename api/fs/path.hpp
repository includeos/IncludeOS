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

namespace fs {

class Path {
public:
  //! constructs Path to the current directory
  Path();

  //! constructs Path to @path
  Path(const std::string& path);
  
  size_t size() const noexcept
  { return stk.size(); }

  const std::string& operator [] (const int i) const noexcept
  { return stk[i]; }

  int getState() const noexcept
  { return state; }
  
  Path& operator = (const std::string& p) {
    stk.clear();
    this->state = parse(p);
    return *this;
  }

  Path& operator += (const std::string& p) {
    this->state = parse(p);
    return *this;
  }

  Path operator + (const std::string& p) const {
    Path np  = Path(*this);
    np.state = np.parse(p);
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
  
  std::string front() const 
  { return stk.front(); }

  std::string back() const
  { return stk.back(); }

  Path& pop_front() noexcept
  { stk.pop_front(); return *this; }
  
  Path& pop_back() noexcept
  { stk.pop_back(); return *this; }
  
  Path& up()
  { if (not stk.empty()) stk.pop_back(); return *this; }

  std::string to_string() const;
  
private:
  int  parse(const std::string& path);
  void name_added(const std::string& name);
  std::string real_path() const;
  
  int state;
  std::deque<std::string> stk;
}; //< class Path
  
} //< namespace fs

#endif //< FS_PATH_HPP
