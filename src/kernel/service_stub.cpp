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

#define ARGS_MAX    64

// name and binary of current service (built from another module)
extern const char* service_binary_name__;
extern const char* service_name__;

const char* Service::binary_name() {
  return service_binary_name__;
}
const char* Service::name() {
  return service_name__;
}

// functions that we can override if we want to
__attribute__((weak))
void Service::start()
{
  const std::string args(os::cmdline_args());
  Service::start(args);
}

#ifndef USERSPACE_KERNEL
extern "C" {
  __attribute__((weak))
  int main(int, const char*[]) {}
}
#else
extern int main(int, const char*[]);
#endif

__attribute__((weak))
void Service::start(const std::string& cmd)
{
  std::string st(cmd); // mangled copy
  int argc = 0;
  const char* argv[ARGS_MAX];

  // Get pointers to null-terminated string
  char* word = (char*) st.c_str();
  char* end  = word + st.size() + 1;
  bool new_word = false;

  for (char* ptr = word; ptr < end; ptr++) {

    // Replace all spaces with 0
    if(std::isspace(*ptr)) {
      *ptr = 0;
      new_word = true;
      continue;
    }

    // At the start of each word, or last byte, add previous pointer to array
    if (new_word or ptr == end - 1) {
      argv[argc++] = word;
      word = ptr; // next arg
      if (argc >= ARGS_MAX) break;
      new_word = false;
    }
  }

  int exit_status = main(argc, argv);
  INFO("main","returned with status %d", exit_status);
}

__attribute__((weak))
void Service::ready() {}

__attribute__((weak))
void Service::stop() {}
