#include <os>
#include <service>

void Service::start(const std::string& args){
  printf("Args = %s\n", args.c_str());
  printf("Try giving the service less memory, eg. 5MB in vm.json\n");
}
