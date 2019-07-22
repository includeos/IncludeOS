
#include <os>
#include <cstdint>

bool verb = true;

#define MYINFO(X,...) INFO("Service", X, ##__VA_ARGS__)

void Service::start(const std::string& args)
{
  MYINFO("Testing kernel modules. Args: %s", args.c_str());

  auto mods = os::modules();

  for (auto mod : mods) {
    INFO2("* %s @ 0x%x - 0x%x, size: %ib",
           reinterpret_cast<char*>(mod.params),
          mod.mod_start, mod.mod_end, mod.mod_end - mod.mod_start);
  }

  CHECKSERT(mods.size() == 1, "Found %zu modules", mods.size());

  printf("SUCCESS\n");
}
