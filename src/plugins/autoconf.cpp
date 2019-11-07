
#include <os.hpp>
#include <autoconf>

__attribute__((constructor))
static void register_autoconf_plugin() {
  os::register_plugin(autoconf::run, "Autoconf plugin");
}
