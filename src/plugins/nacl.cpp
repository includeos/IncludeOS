
#include <os>

extern void register_plugin_nacl();

__attribute__((constructor))
void register_nacl() {
  os::register_plugin(register_plugin_nacl, "NaCl");
}
