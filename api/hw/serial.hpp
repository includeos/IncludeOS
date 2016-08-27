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

namespace hw{

  class Serial {
      
  public:
    
    /** On Data handler. Return value indicates if the buffer should be flushed **/
    using on_data_handler = delegate<void(char c)>;
    using on_string_handler = delegate<void(const std::string s)>;

    using irq_delg = delegate<void()>;
    
    template <uint16_t PORT>
    static Serial& port(){
      static Serial s{PORT};
      return s;
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
    
    void init();    
    
  private:
    
    Serial(int port);
    static constexpr uint16_t ports_[] {0x3F8, 0x2F8, 0x3E8, 0x2E8 };    
    static constexpr uint8_t irqs_[] {4, 3, 15, 15 };    
    
    //static const char default_newline = '\r';
    char newline = '\r'; //default_newline;

    int nr_{0};
    int port_{0};
    uint8_t irq_{4};
    std::string buf{};

    on_data_handler on_data_ = [](char c){ debug("Default on_data: %c \n", c); (void)c; };
    on_string_handler on_readline_ = [](std::string s) { (void)s; };
    
    void irq_handler_ ();
    void readline_handler_(char c);
  };
  
}

#endif
