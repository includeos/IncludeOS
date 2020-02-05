
#include <os>
#include <net/interfaces>
#include <profile>
#include <cstdio>
#include "liu.hpp"

using storage_func_t = liu::LiveUpdate::storage_func;
extern storage_func_t begin_test_boot();

void Service::start()
{
#ifdef ENABLE_PROFILERS
  auto prof = ScopedProfiler::get_statistics(false);
  printf("%s\n", prof.c_str());
#endif
  os::set_panic_action(os::Panic_action::halt);

  auto func = begin_test_boot();

  if (liu::LiveUpdate::os_is_liveupdated() == false)
  {
    auto& inet = net::Interfaces::get(0);
    inet.network_config({10,0,1,59}, {255,255,255,0}, {10,0,1,1});
    setup_liveupdate_server(inet, 666, func);

    // signal test.py that the server is up
    const char* sig = "Ready to receive binary blob\n";
    os::default_stdout(sig, strlen(sig));
  }
}
