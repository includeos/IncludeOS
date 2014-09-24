#ifndef CLASS_DEV_H
#define CLASS_DEV_H


#include <os>
#include <class_nic.hpp>

/** @todo Impement */
class Disk;
class Serial;
class PIT;
class APIC;


#define LEN_DEVNAME 50
#define MAX_NICS 4
#define MAX_DISKS 4 
#define MAX_SERIALS 4


/**
   Access point for devices
  
   Get a nic by calling `Dev::eth(n)`, a disk by calling `Dev::disk(n)` etc.
*/
class Dev{

  //Private pointer to the device lists
  static Nic* nics[MAX_NICS];
  static Disk* disks[MAX_DISKS];
  static Serial* serials[MAX_SERIALS];

public:
  
  //! Get ethernet device n
  static Nic& eth(int n);
  
  //! Get disk n
  static Disk& disk(int n);  

  //! Get serial port n
  static Serial& serial(int n);
  
  //! Add a nic
  static void add(Nic* n);
  
  //! Add a disk
  static void add(Disk* d);
  
};




#endif

