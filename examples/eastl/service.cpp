#include <os>

extern void tests();
extern int  STREAM_main();


void Service::start()
{
  std::cout << "*** Service is up - with OS Included! ***" << std::endl;
  
  // do the STREAM test here
  STREAM_main();
  
  std::cout << "Service out!" << std::endl;
}
