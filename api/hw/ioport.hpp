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

namespace hw {

  /** Receive a byte from port.
      @param port : The port number to receive from
  */
  static inline uint8_t inb(int port)
  {
    int ret;
    __asm__ volatile ("xorl %eax,%eax");
    __asm__ volatile ("inb %%dx,%%al":"=a" (ret):"d"(port));

    return ret;
  }

  /** Send a byte to port.
      @param port : The port to send to
      @param data : One byte of data to send to @param port
  */
  static inline void outb(int port, uint8_t data) {
    __asm__ volatile ("outb %%al,%%dx"::"a" (data), "d"(port));
  }

  /** Receive a word from port.
      @param port : The port number to receive from
  */
  static inline uint16_t inw(int port)
  {
    int ret;
    __asm__ volatile ("xorl %eax,%eax");
    __asm__ volatile ("inw %%dx,%%ax":"=a" (ret):"d"(port));

    return ret;
  }

  /** Send a word to port.
      @param port : The port to send to
      @param data : One word of data to send to @param port
  */
  static inline void outw(int port, uint16_t data) {
    __asm__ volatile ("outw %%ax,%%dx"::"a" (data), "d"(port));
  }

} //< namespace hw

#endif // HW_IOPORT_HPP
