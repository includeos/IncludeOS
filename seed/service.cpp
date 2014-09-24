#include <os>
#include <class_dev.hpp>

void Service::start(){
  printf("\n *** Service is up - with OS Included! *** \n");

  auto eth1=Dev::eth(0);
  //auto eth2=Dev::eth<VirtioNet>(1);
  printf("Using ethernet device: %s \n", eth1.name());
  
}
