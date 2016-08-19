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
#include <cassert>
#include <smp>

void Service::start(const std::string&)
{
  static int completed = 0;
  static uint32_t job = 0;
  static const int TASKS = 8 * sizeof(job);
  
  // schedule tasks
  for (int i = 0; i < TASKS; i++)
  SMP::add_task(
  [i] {
    // the job
    __sync_fetch_and_or(&job, 1 << i);
  }, 
  [i] {
    // job completion
    completed++;
    
    if (completed == TASKS) {
      printf("All jobs are done now, compl = %d\n", completed);
      printf("bits = %#x\n", job);
      assert(job = 0xffffffff && "All 32 bits must be set");
    }
  });
  // start working on tasks
  SMP::start();
  
  printf("*** TEST SERVICE STARTED *** \n");
}
