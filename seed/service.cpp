#include <os>
#include <iostream>

using namespace std;

void Service::start()
{
  cout << "*** Service is up - with OS Included! ***" << endl;
    
  auto eth1=Dev::eth(0);
  
  cout << "Using ethernet device:" << eth1.name() << endl;  
  cout << "Mac: " << eth1.mac_str() << endl;
  cout << "Service out! " << endl;
}
