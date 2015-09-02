#include <os>

void Service::start() {
  //srand(time(NULL));
  //volatile int i = rand();
  //printf(">!<\n");
    
  //printf("i: %i \n",i);
  //std::cout << "test " << std::endl;
  
  std::set_terminate([](){ printf("CUSTOM TERMINATE Handler \n"); });
  std::set_new_handler([](){ printf("CUSTOM NEW Handler \n"); });

  try {
    printf("TRY \n");
    if (OS::uptime() > 0.1)
      throw std::runtime_error("Crazy Error!");
  }catch(std::runtime_error e){
    
    printf("Caught runtime error: %s \n",e.what());
    
  }catch(int i) {
    
    printf("Caught int %i \n",i);
  
  }
  
  printf("*** DONE *** \n");
 

}
