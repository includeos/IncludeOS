#include <os>

int my_init_functions = 0;
int f1_data = 0;

void func1(){
  INFO("Custom 1","initialization function 1");
  f1_data = 0xf1;
  my_init_functions++;
}

struct Autoreg {
  Autoreg() {
    OS::register_custom_init(func1, "Custom 1");
  }
} autoregister;
