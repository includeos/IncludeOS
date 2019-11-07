
/**
 * Example plugin. Also used for testing.
 */

#include <os>
#include <kprint>

bool example_plugin_registered = false;
bool example_plugin_run = false;

// The actual plugin.
void example_plugin(){
  INFO("Example plugin","initializing");
  Expects(example_plugin_registered);
  example_plugin_run = true;
}


// Run as global constructor
__attribute__((constructor))
void register_example_plugin(){
  os::register_plugin(example_plugin, "Example plugin");
  example_plugin_registered = true;
  // Note: printf might rely on global ctor and may not be ready at this point.
  INFO("Example", "plugin registered");
}
