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

#include <hw/serial.hpp>
#include <kernel/events.hpp>

using namespace hw;

// Storage for port numbers
constexpr uint16_t Serial::PORTS[];
constexpr uint8_t  Serial::IRQS[];

void Serial::init(uint16_t port_) {
  hw::outb(port_ + 1, 0x00);    // Disable all interrupts
  hw::outb(port_ + 3, 0x80);    // Enable DLAB (set baud rate divisor)
  hw::outb(port_ + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
  hw::outb(port_ + 1, 0x00);    //                  (hi byte)
  hw::outb(port_ + 3, 0x03);    // 8 bits, no parity, one stop bit
  hw::outb(port_ + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
  hw::outb(port_ + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

Serial::Serial(int port) :
  port_(port < 5 ? PORTS[port-1] : 0),
  irq_(IRQS[port-1])
{
  static bool initialized = false;
  if (!initialized) {
    initialized = true;
    init();
  }
}

void Serial::print_handler(const char* str, size_t len) {
  for(size_t i = 0; i < len; ++i)
      this->write(str[i]);
}

void Serial::on_data(on_data_handler del) {
  enable_interrupt();
  on_data_ = del;
  INFO("Serial", "Subscribing to data on IRQ %i",irq_);
  Events::get().subscribe(irq_, {this, &Serial::irq_handler_});
  __arch_enable_legacy_irq(irq_);
}

void Serial::on_readline(on_string_handler del, char delim) {
  newline = delim;
  on_readline_ = del;
  on_data({this, &Serial::readline_handler_});
  debug("<Serial::on_readline> Subscribing to data %i \n", irq_);
}

void Serial::enable_interrupt() {
  outb(port_ + 1, 0x01);
}

void Serial::disable_interrupt() {
  outb(port_ + 1, 0x00);
}

char Serial::read() {
  return hw::inb(port_);
}

void Serial::write(char c) {
  while (is_transmit_empty() == 0);
  hw::outb(port_, c);
}

int Serial::received() {
  return hw::inb(port_ + 5) & 1;
}

int Serial::is_transmit_empty() {
  return hw::inb(port_ + 5) & 0x20;
}

void Serial::irq_handler_ () {

  while (received())
    on_data_(read());

}

void Serial::readline_handler_ (char c) {

  if (c != newline) {
    buf.append(1, c);
    write(c);
    return;
  }

#ifdef DEBUG
  for ( auto& ch : buf )
    debug(" 0x%x | ", ch);
#endif

  // Call the event handler
  on_readline_(buf);
  buf.clear();
}

void Serial::EOT() {
  outb(PORTS[0], 0x4);
}
