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

#include <hw/kbm.hpp>
#include <kernel/irq_manager.hpp>

#define PS2_DATA_PORT   0x60
#define PS2_STATUS      0x64
#define PS2_COMMAND     0x64

#define CMD_DISABLE_SCANNING  0xF5

#define CMD_DISABLE_PORT1     0xAD
#define CMD_ENABLE_PORT1      0xAE
#define CMD_DISABLE_PORT2     0xA7
#define CMD_ENABLE_PORT2      0xA8


#define TYPE_PS2_MOUSE    0x00
#define TYPE_MOUSE_WHEEL  0x03
#define TYPE_5_BTN_MOUSE  0x04
#define TYPE_KEYB_MF2     0xAB

namespace hw
{
  bool key_press[256];
  int  mouse_x;
  int  mouse_y;
  bool mouse_button[4];
  
  uint8_t ps2_read()
  {
    return hw::inb(PS2_DATA_PORT);
  }
  
  void KBM::init()
  {
    printf("registering ps/2 keyboard interrupt\n");
    bsp_idt.subscribe(1,
    [] {
      printf("keyboard interrupt\n");
    });
    bsp_idt.subscribe(9,
    [] {
      printf("m interrupt\n");
    });
    
    // disable scanning
    hw::outb(PS2_COMMAND, CMD_DISABLE_SCANNING);
    
    // disable ports
    hw::outb(PS2_COMMAND, CMD_DISABLE_PORT1);
    hw::outb(PS2_COMMAND, CMD_DISABLE_PORT2);
    
    // discard old data
    ps2_read();
    
    // enable ports
    hw::outb(PS2_COMMAND, CMD_ENABLE_PORT1);
    hw::outb(PS2_COMMAND, CMD_ENABLE_PORT2);
    
    hw::outb(PS2_COMMAND, 0x60);
    hw::outb(PS2_COMMAND, 0x1 | 0x2);
    
    hw::outb(PS2_COMMAND, 0xF4);
    
  }
}
