// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <string.h>
#include <errno.h>
#include <info>

extern "C" void test_sysconf();
extern "C" void test_pathconf();
extern "C" void test_pwd();

int main(int, char **) {

  struct utsname name;

  printf("System info:\n");
  int res = uname(&name);
  if (res == -1 ) {
    printf("uname error: %s", strerror(errno));
  }
  printf("%s %s\n", name.sysname, name.version);

  test_sysconf();
  test_pathconf();

  // test environment variables
  char* value = getenv("HOME");
  if (value) printf("HOME: %s\n", value);

  char* new_env = const_cast<char*>("CPPHOME=/usr/home/cpp");
  res = putenv(new_env);
  value = getenv("CPPHOME");
  if (value) printf("CPPHOME: %s\n", value);

  res = setenv("INCLUDEOS_CORE_DUMP", "core.dump", 1);
  value = getenv("INCLUDEOS_CORE_DUMP");
  printf("INCLUDEOS_CORE_DUMP: %s\n", value);

  // test pwd-related functions
  test_pwd();

  INFO("Conf", "SUCCESS");
}
