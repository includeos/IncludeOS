#include <os>
#include <stdio.h>


class CustomException : public std::runtime_error {
  using runtime_error::runtime_error;
};

void Service::start()
{
  
  printf("TESTING Exceptions \n");
  
  const char* error_msg = "Crazy Error!";
  
  try {
    printf("[x] Inside try-block \n");
    if (OS::uptime() > 0.1){
      std::runtime_error myexception(error_msg);
      throw myexception;
    }

  }catch(std::runtime_error e){
    
    printf("[%s] Caught runtime error: %s \n", std::string(e.what()) == std::string(error_msg) ? "x" : " " ,e.what());
    
  }catch(...) {
    
    printf("[ ] Caught something - but not what we expected \n");
    
  }
  
  std::string custom_msg = "Custom exceptions are useful";
  std::string cought_msg = "";
  
  try {
    // Trying to throw a custom exception
    throw CustomException(custom_msg);
  } catch (CustomException e){
    
    cought_msg = e.what();
    
  } catch (...) {
    
    printf("[ ] Couldn't catch custom exception \n");
    
  }
  
  assert(cought_msg == custom_msg);
  printf("[x] Cought custom exception \n");
}
