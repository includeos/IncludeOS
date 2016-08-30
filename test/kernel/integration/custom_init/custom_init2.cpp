#include <os>

extern int my_init_functions;
int f2_data = 0;

void func2(){
  INFO("Custom 2","initialization function 2");
  f2_data = 0xf2;
  my_init_functions++;
}

__attribute__((constructor))
void autoregister(){
    OS::register_custom_init(func2, "Custom 2");
}
