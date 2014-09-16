#ifndef CLASS_DEVICE_HPP
#define CLASS_DEVICE_HPP

#include <os>

#define LEN_DEVNAME 50

//TODO: Make this do something useful

class Device{

public:
  enum bus_t{PCI,IDE,ISA,SCSI};
  Device(bus_t _bustype, int _busno=0);
  int busno();
  
private:
  bus_t bustype;  
  int busnumber;
  
  char name[LEN_DEVNAME];  
  int reads;
  int writes;

};


#endif
