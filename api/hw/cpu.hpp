
#ifndef OS_HW_CPU
#define OS_HW_CPU

#include <util/units.hpp>

namespace os {
  util::KHz cpu_freq();
}

namespace os::experimental::hw {
  struct CPU {
    struct Task;
    auto frequency();
    void add_task(Task t);
    void signal();
    std::vector<std::reference_wrapper<Task>> tasks();
  };
}

#endif
