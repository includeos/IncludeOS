#include "kernel/memory.hpp"
#include <os>
#include <service>

void Service::start(const std::string& args){
  std::println("Hello from the example unikernel!");
  std::println();

  std::println("Current virtual mappings:");
  for (const auto& entry : os::mem::vmmap())
      std::println(" {}", entry.second.to_string());
  std::println();

  std::println("Tip: Try changing how much memory you give to the service in vm.json");
  std::println("Service done. Shutting down...");

  os::shutdown();
}
