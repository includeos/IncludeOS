// Part of the IncludeOS Unikernel  - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud. 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <class_nic.hpp>
#include <class_pci_device.hpp>





/*
template <> const char* Nic<VirtioNet>::name(){
  //return "Fantastic VirtioNic No.1";
  return driver.name();
}
*/

/*
template <> Nic<VirtioNet>::Nic(PCI_Device* _dev)
  : pcidev(_dev) //Device(this)
{
  
  printf("\n Nic at PCI addr 0x%x scanning for resources\n",_dev->pci_addr());
    
  _dev->probe_resources();
  
}

*/

/*
template <> const char* Nic<E1000>::name(){
  return "Specialized E1000 No.1";
}


template <> Nic<E1000>::Nic(PCI_Device* _dev)
  : pcidev(_dev) //Device(this)
{
  
  printf("\n Nic at PCI addr 0x%x scanning for resources\n",_dev->pci_addr());
    
  _dev->probe_resources();
  
}
*/
