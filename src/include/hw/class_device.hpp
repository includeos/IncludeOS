#ifndef CLASS_DEVICE_HPP
#define CLASS_DEVICE_HPP

#include <os>

#define LEN_DEVNAME 50
#define MAX_NICS 4
#define MAX_DISKS 4 

class Nic;
class Disk;

/**
  @brief Public ABI class for device access
  Get a nic by Device::eth(n).down()
 */

class Dev{
  
  //Private pointer to the device lists
  static Nic* nics[MAX_NICS];
  static Disk* disks[MAX_DISKS];
  
public:
  
  //@brief Get ethernet device n
  static Nic& eth(int n);
  
  //@brief Get disk n
  static Disk& disk(int n);  
  
  static void add(Nic* n);
  static void add(Disk* d);
  
};


//Bus types
enum bus_t{PCI,IDE,ISA,SCSI};


/**
   @brief Device specialization class
*/
template<class DEV_T>
class Device{
  //The actual device
  DEV_T dev; 

  int _reads;
  int _writes;
  
public:
  //A way to avoid polymorphism
  const char* name(){
    return dev.name();
  };
  
  Device<DEV_T>(DEV_T* dev){
    Dev::add(dev);
  }
  
};


#endif
