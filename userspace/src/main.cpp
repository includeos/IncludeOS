#include <service>
#include <os.hpp>
#include <kernel.hpp>

struct alignas(4096) page_t {
  char buffer[4096];
};
static std::array<page_t, 1024> machine_pool;
static os::Machine* m4chine = nullptr;
os::Machine& os::machine() noexcept {
  return *m4chine;
}

extern "C"
int userspace_main(int, char** args)
{
  m4chine = os::Machine::create(machine_pool.data(), sizeof(machine_pool));
  // TODO: init machine
  //m4chine->init();

  // initialize Linux platform
  kernel::start(args[0]);

  // calls Service::start
  kernel::post_start();
  return 0;
}

#ifndef LIBFUZZER_ENABLED
// default main (event loop forever)
int main(int argc, char** args)
{
  int res = userspace_main(argc, args);
  if (res < 0) return res;
  // begin event loop
  os::event_loop();
  printf("*** System shutting down!\n");
  return 0;
}
#endif
