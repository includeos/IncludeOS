#include <os>

extern bool example_plugin_registered;
extern bool example_plugin_run;
int my_plugin_functions = 0;
int f1_data = 0;

__attribute__((constructor))
static void func1(){


  // NOTE: since this plugin is a part of the service, its ctors
  // will be called after the installed plugin ctors. Therefore we can
  // depend on installed plugins being run / registered.

  CHECKSERT(example_plugin_registered, "Example plugin registered");
  CHECKSERT(example_plugin_run, "Example plugin ran");

  INFO("Plugin 1","initialization function 1");
  f1_data = 0xf1;
  my_plugin_functions++;
}
