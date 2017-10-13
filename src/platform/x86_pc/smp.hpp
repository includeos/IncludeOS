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

#pragma once
#ifndef X86_SMP_HPP
#define X86_SMP_HPP

#include <cstdint>
#include <vector>
#include <smp>

typedef SMP::task_func smp_task_func;
typedef SMP::done_func smp_done_func;

namespace x86 {

extern void init_SMP();
extern void initialize_gdt_for_cpu(int cpuid);

}

#endif
