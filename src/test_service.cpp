#include <os>

/*template
std::basic_ostream<char, std::char_traits<char> >&
std::basic_ostream<char, std::char_traits<char> >::write(const char* __s, std::streamsize __n); */
/*extern "C" void*  __eh_frame_start;
  extern "C" void __register_frame ( void * );*/
void Service::start() {
  
  std::cout << "test " << std::endl;

  // Wonder when these are used...?
  std::set_terminate([](){ printf("CUSTOM TERMINATE Handler \n"); });
  std::set_new_handler([](){ printf("CUSTOM NEW Handler \n"); });

  //std::__1::basic_ostream<char, std::__1::char_traits<char> >::write(char const*, long);
  std::cout.write("test", 4l);
 
  printf("BUILT WITH CLANG \n");
  
  try {
    printf("TRY \n");
    if (OS::uptime() > 0.1){
      std::runtime_error myexception("Crazy Error!");
      printf("My exception: %s \n",myexception.what());
      throw myexception;
    }
  }catch(std::runtime_error e){
    
    printf("Caught runtime error: %s \n",e.what());
    
  }catch(int i) {
    
    printf("Caught int %i \n",i);
  
  }
  
  printf("*** DONE *** \n");
 

}
