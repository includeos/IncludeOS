#include <os>
#include <class_dev.hpp>

void sse_testing();

void Service::start(){
  printf("\n *** Service is up - with OS Included! *** \n");
  
  auto eth1=Dev::eth(0);
  //auto eth2=Dev::eth<VirtioNet>(1);
  
  printf("Using ethernet device: %s \n", eth1.name());

  printf("Mac: %s \n",eth1.mac_str());
  
  printf("Service out! \n");
}

