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


#include <cstdio>
#include <iostream>
#include <os>


int main(int argc, char** argv) {

  printf("Hello main\n");
  std::cout << "Argc: %i \n" << argc << "\n";

  for (int i = 0; i < argc; i++)
    printf("Arg %i: %s\n", i, argv[i]);

  Expects(std::string(argv[0]) == "test_main");
  Expects(std::string(argv[1]) == "booted");
  Expects(std::string(argv[2]) == "with");
  Expects(std::string(argv[3]) == "vmrunner/Qemu");

  // We want to veirify this "exit status" on the back-end
  return 200;

}
