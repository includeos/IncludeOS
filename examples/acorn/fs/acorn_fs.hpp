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
#ifndef ACORN_FS_HPP
#define ACORN_FS_HPP

#include <memdisk>
#include <fs/disk.hpp>

namespace acorn {

using Disk_ptr = fs::Disk_ptr;

void recursive_fs_dump(Disk_ptr disk, const std::vector<fs::Dirent>& entries, const int depth = 1);

void list_static_content(Disk_ptr disk);

} //< namespace acorn

#endif //< ACORN_FS_HPP
