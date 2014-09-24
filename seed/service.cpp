#include <os>

void Service::start(){
  printf("\n *** Service is up - with OS Included! *** \n");

  printf("Using ethernet device: %s \n", Dev::eth(0).name());
  
}
