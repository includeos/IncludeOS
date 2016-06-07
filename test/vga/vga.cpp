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

#include <os>
#include <stdio.h>
#include <info>
#include <vga>
ConsoleVGA vga;

static char c = '0';
static int iterations = 0;

using namespace std::chrono;

void write_goodbye(){

  char msg[] = {0xd,0xe,' ','G','A','M','E',' ','O','V','E','R',' ',0xe,0xd,0};  
  vga.setCursorAt(32,24);
  vga.setColor(ConsoleVGA::COLOR_LIGHT_BROWN);
  vga.write(msg, sizeof(msg));
  
}

void Service::start()
{
  INFO("VGA", "Running tests for VGA");
  
  OS::set_rsprint([] (const char* data, size_t len) {
      vga.write(data, len);
    });
  
  printf("Hello there!\n");
  
  auto test1 = [](){
    vga.putEntryAt('@',0,0);
    vga.putEntryAt('@',0,24);
    vga.putEntryAt('@',79,0);
    vga.putEntryAt('@',79,24);
    
    static int row = 0;
    static int col = 0;

    vga.setColor(col % 256);
    vga.putEntryAt(c,col % 80, row % 25);
    
    if (col++ % 80 == 79){      
      row++;
    }
    
    if (row % 25 == 24 and col % 80 == 79)
      c++;
  };  

  
  auto test1_1 = [] () -> bool
    {
      if ( c >= '4') {
        hw::PIT::instance().onRepeatedTimeout(100ms, [] {
            vga.newline();
            iterations++;
            if (iterations == 24)
              write_goodbye();
          
          },
        
          [] {
            return iterations < 36;
          });
      }
      return c < '4';
    };

  
  auto test2 = [](){
    const int width = 40;
    
    char buf[width];
    for (int i = 0; i<width; i++)
      buf[i] = c;
    
    buf[width - 1] = '\n';
    vga.write(buf,width);
    c++;
  };

  auto test3 = [test1,test1_1](){    
    for (uint8_t i=0; i<255; i++){
      vga.setColor(i);
      vga.write('#');
    }
    vga.setColor(vga.make_color(ConsoleVGA::COLOR_WHITE, ConsoleVGA::COLOR_BLACK));
    hw::PIT::instance().onRepeatedTimeout(1ms, test1, test1_1);
  };

  
  hw::PIT::instance().onTimeout(1s, test3);
  
  
  INFO("VGA", "SUCCESS");
}
