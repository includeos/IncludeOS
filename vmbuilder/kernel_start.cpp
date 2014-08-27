#include "class_os.h"

int main();


extern "C" {
  void _start(void){    
    main();
  }

  int main(){
    OS::start();
    //OBS: If this function returns, the consequences are UNDEFINED
    return 0;
  }
  
}


