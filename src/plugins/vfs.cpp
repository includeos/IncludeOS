// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 Oslo and Akershus University College of Applied Sciences
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

// This file contains a plugin for automatically mounting
// resources to the virtual filesystem (VFS)

#include <fs/vfs.hpp>
#include <posix/fd_map.hpp>

#include <rng_fd.hpp>
// Mount RNG functionality to system paths
static void mount_rng()
{
  RNG::get().open_fd = []()->FD& {
    return FD_map::_open<RNG_fd>();
  };

  fs::mount("/dev/random", RNG::get(), "Random device");
  fs::mount("/dev/urandom", RNG::get(), "Random device");
}

// Function being run by the OS for mounting resources
static void vfs_mount()
{
  // RNG
  mount_rng();
}

#include <os>
__attribute__((constructor))
static void vfs_mount_plugin() {
  OS::register_plugin(vfs_mount, "VFS Mounter");
}
