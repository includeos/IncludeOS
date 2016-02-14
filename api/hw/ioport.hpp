// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
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

#ifndef HW_IOPORT_HPP
#define HW_IOPORT_HPP

#include <common>

class IOport{
  
	public:     
  /** Receive a byte from port.
      @param port : The port number to receive from
  */
  static uint8_t inb(int port);
  
  /** Send a byte to port.
      @param port : The port to send to
	  @param data : One byte of data to send to @param port
  */
  static void outb(int port, uint8_t data);
};

#endif
