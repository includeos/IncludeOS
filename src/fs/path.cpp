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

#include <fs/path.hpp>

#include <string>
#include <cerrno>

namespace fs
{
  static const char PATH_SEPARATOR = '/';

  Path::Path()
    : Path("/")
  {
    // uses current directory
  }

  Path::Path(const std::string& path)
  {
    // parse full path
    this->state_ = parse_add(path);

  } // Path::Path(std::string)

  Path::Path(std::initializer_list<std::string> parts)
  {
    for (auto part : parts)
      parse_add(part);
  }

  std::string Path::to_string() const
  {
    // build path
    std::string ss;
    for (const auto& p : this->stk)
      {
        ss += PATH_SEPARATOR + p;
      }
    // append path/ to end
    ss += PATH_SEPARATOR;
    return ss;
  }

  int Path::parse_add(const std::string& path)
  {
    if (path.empty())
      {
        // do nothing?
        return 0;
      }

    std::string buffer(path.size(), 0);
    char lastChar = 0;
    int  bufi = 0;

    for (size_t i = 0; i < path.size(); i++)
      {
        if (path[i] == PATH_SEPARATOR)
          {
            if (lastChar == PATH_SEPARATOR)
              { // invalid path containing // (more than one forw-slash)
                return -EINVAL;
              }
            if (bufi)
              {
                name_added(std::string(buffer, 0, bufi));
                bufi = 0;
              }
            else if (i == 0)
              {
                // if the first character is / separator,
                // the path is relative to root, so clear stack
                stk.clear();
              }
          }
        else
          {
            buffer[bufi] = path[i];
            bufi++;
          }
        lastChar = path[i];
      } // parse path
    if (bufi)
      {
        name_added(std::string(buffer, 0, bufi));
      }
    return 0;
  }

  void Path::name_added(const std::string& name)
  {
    if (name == ".")
      return;

    stk.push_back(name);

  }
}
