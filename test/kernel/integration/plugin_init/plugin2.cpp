#include <os>

extern int my_plugin_functions;
int f2_data = 0;

__attribute__((constructor))
static void func2(){
  INFO("Plugin 2","initialization function 2");
  f2_data = 0xf2;
  my_plugin_functions++;
}
