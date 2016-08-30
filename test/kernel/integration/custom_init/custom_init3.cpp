#include <os>
#include <stdexcept>

extern int my_init_functions;
int f3_data = 0;

// The OS will catch std::exception and print "what" on error.
class myexcept : public std::exception {
  using std::exception::exception;  
  const char* what() const noexcept override{
    f3_data = 0xf417;
    return "My custom message";
  }  
};

void func3(){
  INFO("Custom 3","initialization function 3");
  my_init_functions++;
  throw myexcept();
  f3_data = 0xf3;
}

__attribute__((constructor))
void autoregister3(){
    OS::register_custom_init(func3, "Custom 3");
}
