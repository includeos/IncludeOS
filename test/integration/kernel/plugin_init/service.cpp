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

extern int f1_data;
extern int f2_data;
extern int f3_data;
extern int my_plugin_functions;
extern bool example_plugin_registered;
extern bool example_plugin_run;

void Service::start()
{

  INFO("Plugin test", "Testing the plugin initialization");

  CHECKSERT(example_plugin_registered, "Example plugin registered");
  CHECKSERT(example_plugin_run, "Example plugin ran");

  INFO("Plugin test", "Make sure the custom plugin initialization functions were called");

  CHECKSERT(f1_data == 0xf1, "f1 data OK");
  CHECKSERT(f2_data == 0xf2, "f2 data OK");
  CHECKSERT(f3_data == 0xf3, "f3 data OK");
  CHECKSERT(my_plugin_functions == 3, "Correct number of function calls");

  INFO("Plugin test", "SUCCESS");
}
