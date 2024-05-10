
#include <cstdlib>
#include <cstdio>
#include <os.hpp>

void __default_quick_exit() {
  os::panic(">>> Quick exit, default route");
}

// According to the standard this should probably be a list or vector.
static void (*__quick_exit_func)() = __default_quick_exit;

int at_quick_exit (void (*func)())
{
  // Append to the ist
  __quick_exit_func = func;
  return 0;
}

__attribute__((noreturn))
void quick_exit (int status)
{
  // Call the exit-function(s) and then _Exit
  __quick_exit_func();

  printf("\n>>> EXIT_%s (%i) \n",status==0 ? "SUCCESS" : "FAILURE",status);

  // Well.
  os::panic("Quick exit called. ");

  // ...we could actually return to the OS. Like, if we want to stay responsive, answer ping etc.
  // How to clean up the stack? Do we even need to?
}
