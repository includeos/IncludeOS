// Part of the IncludeOS unikernel - www.includeos.org
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

#include <fs/common.hpp>

std::string fs_error_string(int error)
{
  switch (error)
  {
  case 0:
    return "No error";
  case -ENOENT:
    return "No such file or directory";
  case -EIO:
    return "I/O Error";
  case -EEXIST:
    return "File already exists";
  case -ENOTDIR:
    return "Not a directory";
  case -EINVAL:
    return "Invalid argument";
  case -ENOSPC:
    return "No space left on device";
  case -ENOTEMPTY:
    return "Directory not empty";
  
  }
  return "Invalid error code";
}
