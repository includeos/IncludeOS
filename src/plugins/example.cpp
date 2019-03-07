// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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

/**
 * Example plugin. Also used for testing.
 */

#include <os>
#include <kprint>

bool example_plugin_registered = false;
bool example_plugin_run = false;

// The actual plugin.
void example_plugin(){
  INFO("Example plugin","initializing");
  Expects(example_plugin_registered);
  example_plugin_run = true;
}


// Run as global constructor
__attribute__((constructor))
void register_example_plugin(){
  os::register_plugin(example_plugin, "Example plugin");
  example_plugin_registered = true;
  // Note: printf might rely on global ctor and may not be ready at this point.
  INFO("Example", "plugin registered");
}
