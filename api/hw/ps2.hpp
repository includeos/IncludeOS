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

#pragma once
#ifndef HW_PS2_HPP
#define HW_PS2_HPP

#include <functional>
#include <cstdint>

namespace hw
{
  class KBM {
  public:
    typedef std::function<void(int)> on_virtualkey_func;
    typedef std::function<void(int, int, int)> on_mouse_func;
    
    enum {
      VK_UNKNOWN,
      
      VK_ESCAPE,
      VK_1,
      VK_2,
      VK_3,
      VK_4,
      VK_5,
      VK_6,
      VK_7,
      VK_8,
      VK_9,
      VK_0,
      
      VK_BACK,
      VK_TAB,
      VK_ENTER,
      VK_SPACE,
      
      
      VK_UP,
      VK_DOWN,
      VK_LEFT,
      VK_RIGHT,
      
      VK_COUNT
    };
    
    static void init();
    
    static void set_virtualkey_handler(on_virtualkey_func func) {
      get().on_virtualkey = func;
    }
    
    static KBM& get() {
      static KBM kbm;
      return kbm;
    }
    
  private:
    KBM();
    int  mouse_x;
    int  mouse_y;
    bool mouse_button[4];
    
    int transform_ascii(int vk);
    int transform_vk(uint8_t scancode);
    void handle_mouse(uint8_t scancode);
    
    on_virtualkey_func on_virtualkey;
    on_mouse_func      on_mouse;
  };
}

#endif
