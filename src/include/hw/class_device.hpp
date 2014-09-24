#ifndef CLASS_DEVICE_HPP
#define CLASS_DEVICE_HPP

#include <class_dev.hpp>

//Bus types
enum bus_t{PCI,IDE,ISA,SCSI};


/**
   TODO: Device specialization class

   Don't know if we'll need this. What's **important** *and common* 
   for all devices anyway?
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
