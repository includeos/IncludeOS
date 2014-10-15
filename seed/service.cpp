#include <os>
#include <iostream>

using namespace std;

void Service::start()
{
  cout << "*** Service is up - with OS Included! ***" << endl;
    
  //IP_stack& net = Dev::eth(0).ip_stack();
  auto& net = Dev::eth(0).ip_stack();
  /** @note: "auto net" would cause copy-construction (!) 
      since auto drops reference, const and volatile qualifiers. */
  
  //A one-way UDP server (a primitive test)
  net.udp_listen(8080,[](uint8_t* const data,int len){
      
      // stringify (might mess up the ethernet trailer; oh well)
      data[sizeof(UDP::header)+len] = 0; 

      printf("<APP SERVER> Got %i b of data! \n\tPayload: %s \n",
             len, data + sizeof(UDP::header));
      return 0;
    });
  
  cout << "Listening to UDP port 8080 " << endl;
  
  // Hook up to I/O events and do something useful ...
  
  cout << "Service out! " << endl; 
}
