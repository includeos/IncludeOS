#include <os>
#include <service>

void Service::start(const std::string& args){
  printf("Args = %s\n", args.c_str());
  printf("Try giving the service less memory, eg. 10MB in vm.json\n");
  printf("Service done. Shutting down...\n");
  os::shutdown();
}
