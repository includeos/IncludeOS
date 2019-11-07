
#include <os>

extern int f1_data;
extern int f2_data;
extern int f3_data;
extern int my_plugin_functions;
extern bool example_plugin_registered;
extern bool example_plugin_run;

void Service::start()
{

  INFO("Plugin test", "Testing the plugin initialization");

  CHECKSERT(example_plugin_registered, "Example plugin registered");
  CHECKSERT(example_plugin_run, "Example plugin ran");

  INFO("Plugin test", "Make sure the custom plugin initialization functions were called");

  CHECKSERT(f1_data == 0xf1, "f1 data OK");
  CHECKSERT(f2_data == 0xf2, "f2 data OK");
  CHECKSERT(f3_data == 0xf3, "f3 data OK");
  CHECKSERT(my_plugin_functions == 3, "Correct number of function calls");

  INFO("Plugin test", "SUCCESS");
}
