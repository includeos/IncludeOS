
#include <os>
#include <sys/utsname.h>

int main()
{
  struct utsname struct_test;
  CHECKSERT(uname(&struct_test) == 0, "uname with buffer returns no error");
  CHECKSERT(strcmp(struct_test.sysname, "IncludeOS") == 0,
    "sysname is IncludeOS");
  CHECKSERT(strcmp(struct_test.nodename, "IncludeOS-node") == 0,
    "nodename is IncludeOS-node");
  CHECKSERT(strcmp(struct_test.release, os::version()) == 0,
    "release is %s", os::version());
  CHECKSERT(strcmp(struct_test.version, os::version()) == 0,
    "version is %s", os::version());
  CHECKSERT(strcmp(struct_test.machine, ARCH) == 0,
    "machine is %s", ARCH);

  CHECKSERT(uname(nullptr) == -1, "uname with nullptr returns error");
  CHECKSERT(errno == EFAULT, "error is EFAULT");
  return 0;
}

void Service::start(const std::string&)
{
  main();
  printf("SUCCESS\n");
}
