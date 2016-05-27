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
#include <hw/apic.hpp>
#include <kernel/irq_manager.hpp>

#define PS2_DATA_PORT   0x60
#define PS2_STATUS      0x64
#define PS2_COMMAND     0x64

#define CMD_DISABLE_SCANNING  0xF5

#define CMD_DISABLE_PORT1     0xAD
#define CMD_ENABLE_PORT1      0xAE
#define CMD_DISABLE_PORT2     0xA7
#define CMD_ENABLE_PORT2      0xA8

#define DEV_IDENTIFY      0xF2

#define TYPE_PS2_MOUSE    0x00
#define TYPE_MOUSE_WHEEL  0x03
#define TYPE_5_BTN_MOUSE  0x04
#define TYPE_KEYB_MF2     0xAB

#define PORT1_IRQ          1
#define PORT2_IRQ         12

namespace hw
{
  bool key_press[256];
  int  mouse_x;
  int  mouse_y;
  bool mouse_button[4];
  
  typedef void (*port_func)(uint8_t);
  port_func keyboard_write;
  port_func mouse_write;
  
  static inline void ps2_flush()
  {
    while (hw::inb(PS2_STATUS) & 1)
      hw::inb(PS2_DATA_PORT);
  }
  
  static void ctl_send(uint8_t cmd)
  {
    while ((hw::inb(PS2_STATUS) & 2) != 0);
    hw::outb(PS2_COMMAND, cmd);
    while ((hw::inb(PS2_STATUS) & 2) != 0);
  }
  
  static uint8_t read_data()
  {
    while (!(hw::inb(PS2_STATUS) & 1));
    return hw::inb(PS2_DATA_PORT);
  }
  static void send_data(uint8_t cmd)
  {
    while (hw::inb(PS2_STATUS) & 2);
    hw::outb(PS2_DATA_PORT, cmd);
  }
  
  static void write_port1(uint8_t val)
  {
    uint8_t res = 0xFE;
    while (res != 0xFA) {
      send_data(val);
      res = read_data();
    }
  }
  static void write_port2(uint8_t val)
  {
    uint8_t res = 0xFE;
    while (res != 0xFA) {
      ctl_send(0xD4);
      send_data(val);
      res = read_data();
    }
  }
  
  void KBM::init()
  {
    // disable ports
    ctl_send(CMD_DISABLE_PORT1);
    ctl_send(CMD_DISABLE_PORT2);
    ps2_flush();
    
    // configure controller
    ctl_send(0x20);
    uint8_t config = read_data();
    bool second_port = config & (1 << 5);
    config |= 0x1 | 0x2; // enable interrupts
    config &= ~(1 << 6);
    
    // write config
    ctl_send(0x60);
    send_data(config);
    
    ps2_flush();
    
    printf("enable port1\n");
    // enable port1
    ctl_send(CMD_ENABLE_PORT1);
    
    ps2_flush();
    
    // self-test (port1)
    write_port1(0xFF);
    uint8_t selftest = read_data();
    assert(selftest == 0xAA);
    
    write_port1(DEV_IDENTIFY);
    uint8_t id1 = read_data();
    
    printf("port1 type: %#x\n", id1);
    
    if (id1 == 0xAA || id1 == 0xAB) {
      // port1 is the keyboard
      printf("keyboard on port1\n");
      keyboard_write = write_port1;
      mouse_write    = write_port2;
    }
    else {
      // port2 is the keyboard
      printf("keyboard on port2\n");
      mouse_write    = write_port1;
      keyboard_write = write_port2;
    }
    
    // enable keyboard
    keyboard_write(0xF4);
    
    // get and set scancode
    keyboard_write(0xF0);
    send_data(0x01);
    keyboard_write(0xF0);
    send_data(0x00);
    uint8_t scanset = 0xFA;
    while (scanset == 0xFA) scanset = read_data();
    printf("scan set: %#x\n", scanset);
    
    // route and enable interrupt handlers
    const uint8_t KEYB_IRQ = (keyboard_write == write_port1) ? PORT1_IRQ : PORT2_IRQ;
    const uint8_t MOUS_IRQ = (KEYB_IRQ == PORT1_IRQ) ? PORT2_IRQ : PORT1_IRQ;
    assert(KEYB_IRQ != MOUS_IRQ);
    
    // need to route IRQs from IO APIC to BSP LAPIC
    hw::APIC::enable_irq(1);
    hw::APIC::enable_irq(12);
    
    printf("registering ps/2 keyboard interrupt\n");
    bsp_idt.subscribe(32 + KEYB_IRQ,
    [KEYB_IRQ] {
      //printf("keyboard interrupt %u\n", KEYB_IRQ);
      IRQ_manager::eoi(KEYB_IRQ);
      
      printf("%c", read_data());
      
    });
    
    bsp_idt.subscribe(32 + MOUS_IRQ,
    [MOUS_IRQ] {
      printf("mouse interrupt %u\n", MOUS_IRQ);
      IRQ_manager::eoi(MOUS_IRQ);
    });
    
    /*
    // reset and enable keyboard
    send_data(0xF6);
    
    // enable keyboard scancodes
    send_data(0xF4);
    
    // enable interrupts
    //ctl_send(0x60, ctl_read(0x20) | 0x1 | 0x2);
    */
    
    printf("!!! keyboard initialized\n");
  }
}
