#include <hw/class_device.hpp>

int Device::busno(){return busnumber;}

Device::Device(bus_t _bustype, int _busno)
  : bustype(_bustype),busnumber(_busno){};
