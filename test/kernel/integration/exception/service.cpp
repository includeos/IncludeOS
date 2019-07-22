
#include <service>

void Service::start()
{
  // i686
  //asm ("movl $0, %eax");
  //asm ("idivl %eax");
  // x86_64
  asm ("movq $0, %rax");
  asm ("idivq %rax");
}
