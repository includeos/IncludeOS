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
#include <liveupdate>
#include <statman>
#include <unistd.h>

static char* liu_storage_area = nullptr;
static liu::buffer_t read_file(const char* fname)
{
  FILE* f = fopen(fname, "rb");
  assert(f != nullptr && "File not found");

  fseek(f, 0, SEEK_END);
  const long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);  //same as rewind(f);
  assert(fsize > 160 && "Elf files arent that small");

  liu::buffer_t buffer(fsize);
  size_t res = fread(buffer.data(), fsize, 1, f);
  assert(res == 1);
  fclose(f);
  return buffer;
}

static liu::buffer_t         bloberino;
static std::vector<uint64_t> timestamps;
using namespace liu;

static void store_func(Storage& storage, const buffer_t* blob)
{
  timestamps.push_back(os::nanos_since_boot());
  storage.add_vector(0, timestamps);
  assert(blob != nullptr);
  storage.add_buffer(2, *blob);

  auto& stm = Statman::get();
  // increment number of updates performed
  try {
    ++stm.get_by_name("system.updates");
  }
  catch (const std::exception& e)
  {
    ++stm.create(Stat::UINT32, "system.updates");
  }
  stm.store(3, storage);
}
static void restore_func(Restore& thing)
{
  timestamps = thing.as_vector<uint64_t>(); thing.go_next();
  // calculate time spent
  auto t1 = timestamps.back();
  auto t2 = os::nanos_since_boot();
  // set final time
  timestamps.back() = t2 - t1;
  // retrieve binary blob
  bloberino = thing.as_buffer(); thing.go_next();
  // statman
  auto& stm = Statman::get();
  stm.restore(thing); thing.go_next();
  auto& stat = stm.get_by_name("system.updates");
  assert(stat.get_uint32() > 0);
  thing.pop_marker();
}

#include <elf.h>
static void modify_kernel_base(liu::buffer_t& buffer, const char* base)
{
  auto* hdr = (Elf64_Ehdr*) buffer.data();
  assert(hdr->e_ident[EI_MAG0] == ELFMAG0);
  assert(hdr->e_ident[EI_MAG1] == ELFMAG1);
  assert(hdr->e_ident[EI_MAG2] == ELFMAG2);
  assert(hdr->e_ident[EI_MAG3] == ELFMAG3);
  assert(hdr->e_ident[EI_CLASS] == ELFCLASS64);
  auto* phdr = (Elf64_Phdr*) &buffer[hdr->e_phoff];
  phdr->p_paddr = (Elf64_Addr) base;
}

void Service::start()
{
  static const int NUM_ITERATIONS = 100;
  liu_storage_area = new char[64*1024*1024];

  extern bool LIVEUPDATE_USE_CHEKSUMS;
  LIVEUPDATE_USE_CHEKSUMS = true;

  bloberino = read_file("build/linux_liu");
  auto* kernel = new char[bloberino.size()];
  printf("Modifying base address to %p\n", kernel);
  modify_kernel_base(bloberino, kernel);

  for (int tries = 0; tries < NUM_ITERATIONS; tries++)
  {
    try {
      liu::LiveUpdate::register_partition("test", store_func);

      LiveUpdate::exec(bloberino, liu_storage_area);
    }
    catch (liu::liveupdate_exec_success)
    {
      liu::LiveUpdate::resume_from_heap(liu_storage_area, "test", restore_func);
    }
  }

  assert(timestamps.size() == NUM_ITERATIONS);
  // calculate median by sorting
  std::sort(timestamps.begin(), timestamps.end());
  auto median = timestamps[timestamps.size()/2];
  // show information
  printf("Median boot time over %lu samples: %.2f millis\n",
          timestamps.size(), median / 1000000.0);
  /*
  for (auto& stamp : timestamps) {
    printf("%lld\n", stamp);
  }
  */
  for (auto& stat : Statman::get()) {
    printf("%s: %s\n", stat.name(), stat.to_string().c_str());
  }

  delete[] kernel;
  delete[] liu_storage_area;
  os::shutdown();
}
