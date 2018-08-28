// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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

#include <util/config.hpp>
#include <cassert>
#include <cstdio>
#include <memory>

static std::unique_ptr<Config> config = nullptr;
static std::unique_ptr<char[]> buffer = nullptr;

const Config& Config::get() noexcept
{
  if (config == nullptr)
  {
    auto* fp = fopen("config.json", "rb");
    assert(fp != nullptr && "Open config.json in source dir");
    fseek(fp, 0L, SEEK_END);
    long int size = ftell(fp);
    rewind(fp);
    // read file into buffer
    buffer.reset(new char[size+1]);
    size_t res = fread(buffer.get(), size, 1, fp);
    assert(res == 1);
    // config needs null-termination
    buffer[size] = 0;
    // create config
    config = std::unique_ptr<Config> (new Config(buffer.get(), buffer.get() + size));
  }
  assert(config != nullptr);
  return *config;
}
