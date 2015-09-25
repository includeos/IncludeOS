#include <os>
#include <net/inet4>
#include <math.h>
#include <iostream>
#include <sstream>

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

  srand(OS::cycles_since_boot());

  // Get mac-address
  auto& mac = Dev::eth(0).mac();
  
  // Assign an IP-address, using Hårek-mapping :-)
  net::Inet4::ifconfig(net::ETH0, 
		       {{ mac.part[2],mac.part[3],mac.part[4],mac.part[5] }},
		       {{ 255,255,0,0 }} );
   
  // Bring up the interface
  net::Inet4* inet = net::Inet4::up();
  std::cout << "Service IP address: " << net::Inet4::ip4(net::ETH0) << std::endl;
  
  // Set up a server on port 80
  net::TCP::Socket& sock =  inet->tcp().bind(80);

  printf("SERVICE: %i open ports in TCP @ %p \n",
	 inet->tcp().openPorts(), &(inet->tcp()));   
  
  // Assign asynchronous connection handler
  sock.onConnect([](net::TCP::Socket& conn){
      printf("SERVICE got data: %s \n",conn.read(1024).c_str());
      
      int color = rand();
      std::stringstream stream;
      
      std::string ubuntu_medium  = "font-family: \'Ubuntu\', sans-serif; font-weight: 500; ";
      std::string ubuntu_normal  = "font-family: \'Ubuntu\', sans-serif; font-weight: 400; ";
      std::string ubuntu_light  = "font-family: \'Ubuntu\', sans-serif; font-weight: 300; ";
      
      stream << "<html><head>"
	     << "<link href='https://fonts.googleapis.com/css?family=Ubuntu:500,300' rel='stylesheet' type='text/css'>"
	     << "</head><body>"
	     << "<h1 style= \"color: " << "#" << std::hex << (color >> 8) << "\">"	
	     <<  "<span style=\""+ubuntu_medium+"\">Include</span><span style=\""+ubuntu_light+"\">OS</span> </h1>"
	     <<  "<h2>Now speaks TCP!</h2>"
	// .... generate more dynamic content 
	     << "<p>  ...and can improvise http. With limitations of course, but it's been easier than expected so far </p>"
	     << "<footer><hr /> &copy; 2015, Oslo and Akershus University College of Applied Sciences </footer>"
	     << "</body></html>\n";
      
      std::string html = stream.str();
      std::string header="HTTP/1.1 200 OK \n "				\
	"Date: Mon, 01 Jan 1970 00:00:01 GMT \n"			\
	"Server: IncludeOS prototype 4.0 \n"				\
	"Last-Modified: Wed, 08 Jan 2003 23:11:55 GMT \n"		\
	"Content-Type: text/html; charset=UTF-8 \n"			\
	"Content-Length: "+std::to_string(html.size())+"\n"		\
	"Accept-Ranges: bytes\n"					\
	"Connection: keep-alive\n\n";
      
      conn.write(header);
      conn.write(html);
      
      // We don't have to actively close when the http-header says "Connection: close"
      //conn.close();
      
    });



  printf("Checking checksum algs. \n");
  
  const int BUFLEN = 512;
  uint16_t buf[BUFLEN];
  for (int i=0; i<BUFLEN; i++){
    buf[i] = rand();
    
  }

  uint64_t my_llu = 42;
  printf("BUG? My llu is: %llu, and 42 == %i \n",my_llu, 42);  
  
  auto t1 = OS::cycles_since_boot();
  uint16_t sum = net::checksum(((uint16_t*)buf) , 1024);
  auto t2 = OS::cycles_since_boot() - t1;

  assert(t2 < uint32_t(-1));
  
  printf("Checksum (0x%x) took: %llu cycles. \n",sum,(uint32_t)t2);

  auto& time = PIT::instance();
    
  
  // Write a dot every half second
  time.onRepeatedTimeout_ms(500, [](){ printf("."); });

  // Write some more every now and then
  time.onRepeatedTimeout_sec(3, [](){ printf("3 seconds passed..."); });
  
  printf("*** SERVICE STARTED *** \n");
}
