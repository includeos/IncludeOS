#include <os>
#include <stdio.h>

void Service::start()
{
  
  printf("TESTING Quick Exit\n");
  
  at_quick_exit([](){ printf("[x] Custom quick exit-handler called \n"); return; });
  quick_exit(0);
  
  // Make sure we actually exit  (This is dead code -  but for testing)
  at_quick_exit([](){ printf("[0] Service didn't actually exit \n"); return; });
}
