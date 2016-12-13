#include <os>

int my_plugin_functions = 0;
int f1_data = 0;

void func1(){
  INFO("Plugin 1","initialization function 1");
  f1_data = 0xf1;
  my_plugin_functions++;
}

struct Autoreg {
  Autoreg() {
    OS::register_plugin(func1, "Plugin 1");
  }
} autoregister;
