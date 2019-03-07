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

#ifndef HW_SERIAL_HPP
#define HW_SERIAL_HPP

//#define DEBUG

#include <os>
#include <hw/ioport.hpp>
#include <cstdio>

namespace hw {

  class Serial {
  public:
    static constexpr uint16_t PORTS[] {0x3F8, 0x2F8, 0x3E8, 0x2E8 };
    static constexpr uint8_t  IRQS[]  {4, 3, 15, 15 };

    /** On Data handler. Return value indicates if the buffer should be flushed **/
    using on_data_handler = delegate<void(char c)>;
    using on_string_handler = delegate<void(const std::string& s)>;

    template <uint16_t PORT>
    static Serial& port(){
      static Serial s{PORT};
      return s;
    }

    os::print_func get_print_handler() {
      return {this, &Serial::print_handler};
    }

    void on_data(on_data_handler del);
    void on_readline(on_string_handler del, char delim = '\r');

    void enable_interrupt();
    void disable_interrupt();

    char read();
    void write(char c);
    int received();
    int is_transmit_empty();

    // We don't want copies
    Serial( Serial& ) = delete;
    Serial( Serial&& ) = delete;
    Serial& operator=(Serial&) = delete;
    Serial operator=(Serial&&) = delete;

    void init() { init(port_); };

    static void init(uint16_t port);

    /** Direct write to serial port 1 (for early printing before e.g. global ctors) **/
    static void print1(const char* cstr);

    /** Send the EOT ASCII character. Effectively signal EOF to the receiving terminal. **/
    static void EOT();

  private:
    void print_handler(const char*, size_t);

    Serial(int port);
    //static const char default_newline = '\r';
    char newline = '\r'; //default_newline;

    int port_   {PORTS[0]};
    uint8_t irq_{IRQS[0]};
    std::string buf{};

    on_data_handler on_data_       = [](char c){ debug("Default on_data: %c \n", c); (void)c; };
    on_string_handler on_readline_ = [](const std::string& s) { (void)s; };

    void irq_handler_ ();
    void readline_handler_(char c);
  };

}

#endif
