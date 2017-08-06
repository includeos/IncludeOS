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

#include <os>

// the name of the current service (built from another module)
extern "C" {
  __attribute__((weak))
  const char* service_name__ = "(missing service name)";
  __attribute__((weak))
  const char* service_binary_name__ = "(missing binary name)";
}


std::string Service::binary_name() {
  return service_binary_name__;
}

std::string Service::name() {
  return service_name__;
}


// functions that we can override if we want to
__attribute__((weak))
void Service::start()
{
  Service::start(OS::cmdline_args());
}

__attribute__((weak))
void Service::ready() {}

__attribute__((weak))
void Service::stop() {}
