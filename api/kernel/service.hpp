
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
   *  @return: The (descriptive) name of the service
   */
  static const char* name();

  /**
   *  @return: The name of the service binary
   */
  static const char* binary_name();


  /**
   *  The service entry point
   *
   *  This is like an applications 'main' function
   *
   *  @note Whenever this function returns, the OS will be sleeping
   *        until an external interrupt fires (there are no regular timer
   *        interrupts unless you've enabled them).
   */
  static void start();
  static void start(const std::string& cmdline_args);


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
