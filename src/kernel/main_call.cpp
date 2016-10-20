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

#include <service>
#include <info>
#define ARGS_MAX    64

__attribute__((weak))
extern "C" int main(int, const char*[]);

__attribute__((weak))
void Service::start(const std::string& cmd)
{
  std::string st(cmd); // mangled copy
  int argc = 0;
  const char* argv[ARGS_MAX];
  
  // Populate argv
  char* begin = (char*) st.data();
  char* end   = begin + st.size();
  
  for (char* ptr = begin; ptr < end; ptr++)
  if (std::isspace(*ptr)) {
    argv[argc++] = begin;
    *ptr = 0;      // zero terminate
    begin = ptr+1; // next arg
    if (argc >= ARGS_MAX) break;
  }

  int exit_status = main(argc, argv);
  INFO("main","returned with status %d", exit_status);
  //exit(exit_status);
}
