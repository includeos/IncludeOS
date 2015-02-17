
//DEBUG: Comment out
#include <iostream>
#include <stdint.h>
//#include "dnsFormat.hpp"
#include "SimplifiedDnsServer.hpp"
#include "SimplifiedDnsClient.hpp"

class SimplifiedDnsServer;
class SimplifiedDnsClient;

int main()
{
  std::cout << "Starting DNS prototype\n";
  //SimplifiedDnsServer * myDnsServer = new SimplifiedDnsServer();
  //SimplifiedDnsClient * myDnsClient = new SimplifiedDnsClient();
  SimplifiedDnsServer myDnsServer;
  SimplifiedDnsClient myDnsClient;
  myDnsServer.setDnsClient(&myDnsClient);
  myDnsClient.setDnsServer(&myDnsServer);

  myDnsClient.requestNames((uint32_t) 3);
  std::cout << "Main finished...\n";
}

