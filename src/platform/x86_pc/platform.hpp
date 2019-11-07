
#pragma once
#include <delegate>

namespace x86 {
  void register_deactivation_function(delegate<void()> func);
}

extern void __platform_init();
