// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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

#include <os.hpp>
#include <kernel.hpp>

const char* os::arch() noexcept {
  return Arch::name;
}

os::Panic_action os::panic_action() noexcept {
  return kernel::panic_action();
}

void os::set_panic_action(Panic_action action) noexcept {
  kernel::set_panic_action(action);
}

os::Span_mods os::modules()
{
  auto* bootinfo_ = kernel::bootinfo();
  if (bootinfo_ and bootinfo_->flags & MULTIBOOT_INFO_MODS and bootinfo_->mods_count) {

    Expects(bootinfo_->mods_count < std::numeric_limits<int>::max());

    return os::Span_mods {
      reinterpret_cast<os::Module*>(bootinfo_->mods_addr),
        static_cast<int>(bootinfo_->mods_count) };
  }
  return {};
}
