#include "class_pci_manager.hpp"
#include <assert.h>
#include "hw/pci.h"


#define NUM_BUSES 2;
bus* buses=0;
unit* units=0;

const int MAX_UNITS=10;
unit unitlist[MAX_UNITS];
int unitcount=0;

bus* add_bus(unit* self, unsigned long bustype, unsigned long busno){
  //printf("Adding bus, type %lu, nr. %lu \n", bustype, busno);

  //We only need one bus. 
  assert(!buses);   
  buses=new bus{0,0,0,0,0,0,0,0};
  buses->self=self;
  buses->bustype=bustype;
  buses->busno=busno;
  if(self) buses->parent=self->bus;

  return buses;
}

unit* add_unit(bus* bus, unsigned long classcode, unsigned long unitcode, unsigned long unitno){
  //printf("Adding unit, class: 0x%lx, unitcode: 0x%lx, unitno: 0x%lx \n", classcode, unitcode, unitno);
  assert(unitcount<MAX_UNITS);
  unit* newunit=&unitlist[unitcount++];
  newunit->bus = bus;
  newunit->classcode = classcode;
  newunit->unitcode = unitcode;
  newunit->unitno = unitno;
  newunit->vendorname = "ACME Computer Parts Inc.";
  newunit->classname = "Enterprise Class";
  newunit->productname = "ACME 1";

  return newunit;
}

resource* add_resource(unit* unit, unsigned short type, unsigned short flags, unsigned long start, unsigned long len){
  //printf("Adding resource \n");
  return new resource;
}



void PCI_manager::init(){
  printf("\n>>> PCI Manager initializing \n");

  
  //Lots borrowed from "SanOS src/os/dev.c"
  bus* host_bus;
  unsigned long unitcode=get_pci_hostbus_unitcode();
  unit* pci_host_bridge=add_unit(host_bus, PCI_HOST_BRIDGE, unitcode, 0);
  bus* pci_root_bus=add_bus(pci_host_bridge, BUSTYPE_PCI, 0);

  assert(unitcode>0);
  enum_pci_bus(pci_root_bus);
  printf(" >> Found %i PCI units: \n",unitcount);
  for(int i=0;i<unitcount;i++){
    printf("\t * %s, %s from %s \n",
	   unitlist[i].classname, 
	   unitlist[i].productname, 
	   unitlist[i].vendorname);
  }
    
}
