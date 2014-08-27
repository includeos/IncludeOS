#include <os>

//#include <iostream>
//#include <functional>

void Service::start(){
  // int* i=new int(5); //Requires operator new.
  const char* secret="[PASS]\t Scoped variable accessed from Lambda - OK\n";
  
  OS::rsprint("[PASS]\t C++ service class with OS included - OK!\n");
  
  //Lambda, accessing parent scope
  [&](){
    OS::rsprint(secret);
  }();  


  
  /*
    //Exceptions will NOT be working for a while (__cxa_throw, etc. missing)
  try{
    throw 256;
  }catch(int e){
    if(e==256)
      OS::rsprint("[PASS]\t Basic exceptions work");
    else
      OS::rsprint("[FAIL]\t Basic exceptions NOT working");
  }
  */
  sbrk(44);
  
}
