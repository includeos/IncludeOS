#include <os>
#include <stdexcept>

extern int my_plugin_functions;
int f3_data = 0;

// The OS will no longer catch exceptions in plugins
class myexcept : public std::exception {
  using std::exception::exception;
  const char* what() const noexcept override{
    f3_data = 0xf417;
    return "My plugin message";
  }
};

void func3(){
  INFO("Plugin 3","initialization function 3");
  my_plugin_functions++;
  f3_data = 0xf3;
}

__attribute__((constructor))
void autoregister3(){
    OS::register_plugin(func3, "Plugin 3");
}
