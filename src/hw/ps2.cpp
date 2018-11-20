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

#include <hw/ps2.hpp>
#include <hw/ioport.hpp>
#include <arch.hpp>
#include <kernel/events.hpp>
#include <debug>

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
  typedef void (*port_func)(uint8_t);
  static bool      ps2_initialized = false;
  static port_func keyboard_write;
  static port_func mouse_write;

  static inline void ps2_flush()
  {
    while (hw::inb(PS2_STATUS) & 1)
      hw::inb(PS2_DATA_PORT);
  }

  static void ctl_send(uint8_t cmd)
  {
    while (hw::inb(PS2_STATUS) & 2);
    hw::outb(PS2_COMMAND, cmd);
    while (hw::inb(PS2_STATUS) & 2);
  }

  static inline uint8_t read_data()
  {
    while (!(hw::inb(PS2_STATUS) & 1));
    return hw::inb(PS2_DATA_PORT);
  }
  static inline uint8_t read_fast()
  {
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

  KBM::keystate_t KBM::transform_vk(uint8_t scancode)
  {
    keystate_t result;
    result.pressed = (scancode & 0x80) == 0;
    const int scancode7 = scancode & 0x7f;

    if (scancode == 0x0)
    {
      result.key = VK_UNKNOWN;
    }
    else if (scancode7 <= 0x0A)
    {
      result.key = VK_ESCAPE + scancode7 - 1;
    }
    else if (scancode7 == 0x2C)
    {
      result.key = VK_Z;
    }
    else if (scancode7 == 0x2D)
    {
      result.key = VK_X;
    }
    else if (scancode == 0xE0)
    {
      scancode = read_fast();
      result.pressed = (scancode & 0x80) == 0;
      switch (scancode & 0x7f) {
      case 0x48:
        result.key = VK_UP; break;
      case 0x4B:
        result.key = VK_LEFT; break;
      case 0x4D:
        result.key = VK_RIGHT; break;
      case 0x50:
        result.key = VK_DOWN; break;
      default:
        result.key = VK_UNKNOWN;
      }
    }
    else {
      switch (scancode7) {
      case 0x0E:
        result.key = VK_BACK; break;
      case 0x0F:
        result.key = VK_TAB; break;
      case 0x1C:
        result.key = VK_ENTER; break;
      case 0x39:
        result.key = VK_SPACE; break;
      default:
        result.key = VK_UNKNOWN;
      }
    }
    return result;
  }
  void KBM::handle_mouse(uint8_t scancode)
  {
    (void) scancode;
  }

  uint8_t KBM::get_kbd_irq()
  {
    return (keyboard_write == write_port1) ? PORT1_IRQ : PORT2_IRQ;
  }
  KBM::keystate_t KBM::get_kbd_vkey()
  {
    uint8_t byte = read_fast();
    // transform to virtual key
    return transform_vk(byte);
  }
  uint8_t KBM::get_mouse_irq()
  {
    return (keyboard_write == write_port1) ? PORT2_IRQ : PORT1_IRQ;
  }

  void KBM::init()
  {
    if (ps2_initialized) return;
    ps2_initialized = true;

    // disable ports
    ctl_send(CMD_DISABLE_PORT1);
    ctl_send(CMD_DISABLE_PORT2);
    ps2_flush();

    // configure controller
    ctl_send(0x20);
    uint8_t config = read_data();
    bool second_port = config & (1 << 5);
    (void) second_port;

    config |= 0x1 | 0x2; // enable interrupts
    config &= ~(1 << 6);

    // write config
    ctl_send(0x60);
    send_data(config);

    ps2_flush();

    // enable port1
    ctl_send(CMD_ENABLE_PORT1);

    ps2_flush();

    // self-test (port1)
    write_port1(0xFF);
    uint8_t selftest = read_data();
    assert(selftest == 0xAA);

    write_port1(DEV_IDENTIFY);
    uint8_t id1 = read_data();

    if (id1 == 0xAA || id1 == 0xAB) {
      // port1 is the keyboard
      debug("keyboard on port1\n");
      keyboard_write = write_port1;
      mouse_write    = write_port2;
    }
    else {
      // port2 is the keyboard
      debug("keyboard on port2\n");
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

    // route and enable interrupt handlers
    const uint8_t KEYB_IRQ = get_kbd_irq();
    const uint8_t MOUS_IRQ = get_mouse_irq();
    assert(KEYB_IRQ != MOUS_IRQ);

    // need to route IRQs from IO APIC to BSP LAPIC
    __arch_enable_legacy_irq(KEYB_IRQ);
    __arch_enable_legacy_irq(MOUS_IRQ);

    /*
    // reset and enable keyboard
    send_data(0xF6);

    // enable keyboard scancodes
    send_data(0xF4);

    // enable interrupts
    //ctl_send(0x60, ctl_read(0x20) | 0x1 | 0x2);
    */
  }

  KBM::KBM()
  {
    if (ps2_initialized == false) {
      KBM::init();
    }

    const uint8_t KEYB_IRQ = get_kbd_irq();
    const uint8_t MOUS_IRQ = get_mouse_irq();

    Events::get().subscribe(KEYB_IRQ,
    [] {
      keystate_t state = KBM::get_kbd_vkey();
      // call handler
      if (get().on_virtualkey) {
        get().on_virtualkey(state.key, state.pressed);
      }
    });

    Events::get().subscribe(MOUS_IRQ,
    [] {
      get().handle_mouse(read_fast());
    });
  }

}
