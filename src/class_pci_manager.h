#ifndef PCI_MANAGER_H
#define PCI_MANAGER_H

#include "hw/pci.h"

unit* add_unit(bus* bus, unsigned long classcode, unsigned long unitcode, unsigned long unitno);

resource* add_resource(unit* unit, unsigned short type, unsigned short flags, unsigned long start, unsigned long len);

bus* add_bus(unit* self, unsigned long bustype, unsigned long busno);

class PCI_manager{

 public:
  static void init();
};

#endif
