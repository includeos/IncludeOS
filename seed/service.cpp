#include <os>
#include <class_dev.hpp>

void Service::start(){
  printf("\n *** Service is up - with OS Included! *** \n");
  
  auto eth1=Dev::eth(0);
  //auto eth2=Dev::eth<VirtioNet>(1);
  
  printf("Using ethernet device: %s \n", eth1.name());

  const mac_t& mac=eth1.mac();
  printf("Mac: 0x%llx \n",mac);

  /*for (int i = 0; i<6; i++)
    printf(i < 5 ? "%1x." : "%1x \n", mac[i]);*/
    
  printf("Service out! \n");
  
}
