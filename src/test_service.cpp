#include <os>
#include <net/inet4>
#include <math.h>
#include <iostream>

class Test
{
public:
  Test()
  {
    printf("Test() constructor called\n");
  }
  Test(int t)
  {
    printf("Test(%d) constructor called\n", t);
  }
};

Test test;
Test test2(2);

void my_exit(){
  printf("This service has it's own exit routine");
}

void Service::start()
{
  auto& mac = Dev::eth(0).mac();
  net::Inet4::ifconfig(net::ETH0, 
		  {{ mac.part[2],mac.part[3],mac.part[4],mac.part[5] }},
		  {{ 255,255,0,0 }} );
  
  std::cout << "test " << std::endl;
  
  net::Inet4* inet = net::Inet4::up();
  std::cout << "Service IP address: " << net::Inet4::ip4(net::ETH0) << std::endl;
  
  // Wonder when these are used...?
  std::set_terminate([](){ printf("CUSTOM TERMINATE Handler \n"); });
  std::set_new_handler([](){ printf("CUSTOM NEW Handler \n"); });

  //std::__1::basic_ostream<char, std::__1::char_traits<char> >::write(char const*, long);
  //std::cout.write("test", 4l);
 
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
  
  std::cout << "std::cout works and so does" << std::endl;
  std::cout << "std::endl" << std::endl;

  std::vector<int> integers={1,2,3};
  std::map<const char*, int> map_of_ints={std::make_pair("First",42) , std::make_pair("Second",43)};
  
  [] (void) { printf("Hello lambda\n"); } ();
  
  for (auto i : integers)
    printf("Integer %i \n",i);
  
  printf("First from map: %i \n", map_of_ints["First"]);
  printf("Second from map: %i \n", map_of_ints["Second"]);
  
  std::string str = "Hello std::string";
  printf("%s\n", str.c_str());
  
  // TODO: find some implementation for long double, or not... or use double
  //auto sine = sinl(42);
  
  // at_quick_exit(my_exit);
  at_quick_exit([](){ printf("My exit-function uses lambdas! \n"); return; });
  //quick_exit(0);
  
  
  net::TCP::Socket sock =  inet->tcp().bind(80);
  
  printf("*** SERVICE STARTED *** \n");
}
