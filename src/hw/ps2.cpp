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
  static port_func keyboard_write;
  static port_func mouse_write;

  void KBM::flush_data()
  {
    while (hw::inb(PS2_STATUS) & 1)
      hw::inb(PS2_DATA_PORT);
  }
  uint8_t KBM::read_status() {
    return hw::inb(PS2_STATUS);
  }
  uint8_t KBM::read_data() {
    while (not (hw::inb(PS2_STATUS) & 1));
    return hw::inb(PS2_DATA_PORT);
  }
  void    KBM::write_cmd(uint8_t cmd) {
    while (hw::inb(PS2_STATUS) & 2);
    hw::outb(PS2_COMMAND, cmd);
    while (hw::inb(PS2_STATUS) & 2);
  }
  void    KBM::write_data(uint8_t data) {
    while (hw::inb(PS2_STATUS) & 2);
    hw::outb(PS2_DATA_PORT, data);
  }
  void KBM::write_port1(uint8_t data)
  {
    uint8_t res = 0;
    while (res != 0xFA) {
      write_data(data);
      res = read_data();
    }
  }
  void KBM::write_port2(uint8_t data)
  {
    uint8_t res = 0;
    while (res != 0xFA) {
      write_cmd(0xD4);
      write_data(data);
      res = read_data();
    }
  }

  KBM::keystate_t KBM::process_vk()
  {
    uint8_t scancode = m_queue.front();
    //printf("Scancode: %02X\n", scancode);
    keystate_t result;
    result.pressed = (scancode & 0x80) == 0;
    const int scancode7 = scancode & 0x7f;
    // numbers
    if (scancode7 > 0x1 && scancode7 <= 0x0A)
    {
      result.key = VK_ESCAPE + scancode7 - 1;
      m_queue.pop_front();
    }
    else if (scancode == 0xE0)
    {
      // multimedia keys (2-byte scancodes)
      if (m_queue.size() < 2) {
        result.key = VK_WAIT_MORE;
        return result;
      }
      scancode = m_queue.at(1);
      result.pressed = (scancode & 0x80) == 0;
      //printf("MM scancode: %02X\n", scancode);
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
      m_queue.pop_front();
      m_queue.pop_front();
    }
    else
    {
      // single-byte scancodes
      switch (scancode7) {
      case 0x0:
        result.key = VK_UNKNOWN;
      case 0x1:
        result.key = VK_ESCAPE;
      case 0x0E:
        result.key = VK_BACK; break;
      case 0x0F:
        result.key = VK_TAB; break;
      case 0x1C:
        result.key = VK_ENTER; break;
      case 0x39:
        result.key = VK_SPACE; break;
      case 0x2C:
        result.key = VK_Z; break;
      case 0x2D:
        result.key = VK_X; break;
      default:
        result.key = VK_UNKNOWN;
      }
      m_queue.pop_front();
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
  void KBM::kbd_process_data()
  {
    // get new scancodes
    while (hw::inb(PS2_STATUS) & 0x1) {
      m_queue.push_back(hw::inb(PS2_DATA_PORT));
    }
    while (!m_queue.empty())
    {
      auto state = process_vk();
      if (state.key == VK_WAIT_MORE) break;
      // call handler
      if (get().on_virtualkey) {
        get().on_virtualkey(state.key, state.pressed);
      }
    }
  }
  uint8_t KBM::get_mouse_irq()
  {
    return (keyboard_write == write_port1) ? PORT2_IRQ : PORT1_IRQ;
  }

  void KBM::init()
  {
    get().internal_init();
  }
  void KBM::internal_init()
  {
    if (this->m_initialized) return;
    this->m_initialized = true;

    // disable ports
    //write_cmd(CMD_DISABLE_PORT1);
    //write_cmd(CMD_DISABLE_PORT2);
    //flush_data();

    // configure controller
    write_cmd(0x20);
    uint8_t config = read_data();

    // enable port1 and port2 interrupts
    config |= 0x1 | 0x2;
    // remove bit6: port1 translation
    config &= ~0x40;
    // dual channel with mouse
    this->mouse_enabled = config & 0x20;

    // write config back
    write_cmd(0x60);
    write_data(config);
    flush_data();

    // enable port1
    write_cmd(CMD_ENABLE_PORT1);
    flush_data();

    // self-test (port1)
    write_port1(0xFF);
    const uint8_t selftest = read_data();
    assert(selftest == 0xAA && "PS/2 controller self-test");

    write_port1(DEV_IDENTIFY);
    uint8_t id1 = read_data();

    if (id1 == 0xAA || id1 == 0xAB) {
      // port1 is the keyboard
      keyboard_write = write_port1;
      mouse_write    = write_port2;
    }
    else {
      // port2 is the keyboard
      mouse_write    = write_port1;
      keyboard_write = write_port2;
    }

    // disable scanning
    keyboard_write(0xF5);
    // set scancode set 1
    keyboard_write(0xF0);
    write_data(0x01);
    keyboard_write(0xF0);
    write_data(0x00);
    uint8_t scanset = 0xFA;
    while (scanset == 0xFA) scanset = read_data();
    assert(scanset == 0x1);
    // enable scanning
    keyboard_write(0xF4);

    // route and enable interrupt handlers
    const uint8_t KEYB_IRQ = get_kbd_irq();
    // need to route IRQs from IO APIC to BSP LAPIC
    __arch_enable_legacy_irq(KEYB_IRQ);

    if (this->mouse_enabled)
    {
      const uint8_t MOUS_IRQ = get_mouse_irq();
      assert(KEYB_IRQ != MOUS_IRQ);
      __arch_enable_legacy_irq(MOUS_IRQ);
    }
  }

  KBM::KBM()
  {
    internal_init();

    Events::get().subscribe(KBM::get_kbd_irq(),
    [] {
      get().kbd_process_data();
    });
    return;
    // TODO: implement
    Events::get().subscribe(KBM::get_mouse_irq(),
    [] {
      get().handle_mouse(read_data());
    });
  }
}
