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

#ifndef KERNEL_SERVICE_HPP
#define KERNEL_SERVICE_HPP

#include <string>

/**
 *  This is where you take over
 *
 *  The service gets started whenever the OS is done initializing
 */
class Service {
public:
  /**
   *  Get the name of the service
   *
   *  @return: The name of the service
   */
  static std::string name();
  
  /**
   *  The service entry point
   *
   *  This is like an applications 'main' function
   *  
   *  @note Whenever this function returns, the OS will be sleeping
   *        until an external interrupt fires (there are no regular timer
   *        interrupts unless you've enabled them).
   */
  static void start(const std::string&);

  /**
   * Returns the command-line provided to multiboot
  **/
  static const std::string& command_line();

  /**
   * Ready is called when the kernel is done calibrating stuff and won't be 
   * doing anything on its own anymore
  **/
  static void ready();

  /**
   *  Graceful shutdown
   *
   *  If the virtual machine running your service gets a poweroff signal
   *  (i.e. from the hypervisor, like Qemu or VirtualBox) this function should
   *  ensure a safe shutdown.
   */
  static void stop();
}; //< Service

#endif //< KERNEL_SERVICE_HPP
