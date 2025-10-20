// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2018 Oslo and Akershus University College of Applied Sciences
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
#include <sys/utsname.h>

int main()
{
  struct utsname struct_test;
  CHECKSERT(uname(&struct_test) == 0, "uname with buffer returns no error");
  CHECKSERT(strcmp(struct_test.sysname, "IncludeOS") == 0,
    "sysname is IncludeOS");
  CHECKSERT(strcmp(struct_test.nodename, "IncludeOS-node") == 0,
    "nodename is IncludeOS-node");
  CHECKSERT(strcmp(struct_test.release, os::version()) == 0,
    "release is %s", os::version());
  CHECKSERT(strcmp(struct_test.version, os::version()) == 0,
    "version is %s", os::version());
  CHECKSERT(strcmp(struct_test.machine, ARCH) == 0,
    "machine is %s", ARCH);

  CHECKSERT(uname(nullptr) == -1, "uname with nullptr returns error");
  CHECKSERT(errno == EFAULT, "error is EFAULT");
  return 0;
}

void Service::start(const std::string&)
{
  main();
  printf("SUCCESS\n");
}
