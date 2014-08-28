#include "class_os.h"

int main();

extern "C" void _init();

extern "C" {
  void _start(void){    
    OS::rsprint(" \n *** IncludeOS Starting *** \n");
    _init();
    OS::rsprint("IncludeOS Initialized. Calling main\n");
    main();
    //    _fini();

  }

  int main(){
    OS::start();
    //OBS: If this function returns, the consequences are UNDEFINED
    return 0;
  }
  
}


