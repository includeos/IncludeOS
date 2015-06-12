#include <os>

extern int  main();

void Service::start()
{
  std::cout << "*** Service is up - with OS Included! ***" << std::endl;
  
  // do the STREAM test here
  main();
  
  std::cout << "Service out!" << std::endl;
}
