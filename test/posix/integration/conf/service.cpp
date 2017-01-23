#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <string.h>
#include <errno.h>
#include <info>

extern "C" void test_sysconf();

int main(int argc, char *argv[]) {

  struct utsname name;

  printf("System info:\n");
  int res = uname(&name);
  if (res == -1 ) {
    printf("uname error: %s", strerror(errno));
  }
  printf("%s %s\n", name.sysname, name.version);

  test_sysconf();

  // test environment variables
  char* value = getenv("HOME");
	printf("HOME: %s\n", value);

	char* new_env = "CPPHOME=/usr/home/cpp";
	res = putenv(new_env);
	value = getenv("CPPHOME");
	printf("CPPHOME: %s\n", value);

	res = setenv("INCLUDEOS_CORE_DUMP", "core.dump", 1);
	value = getenv("INCLUDEOS_CORE_DUMP");
	printf("INCLUDEOS_CORE_DUMP: %s\n", value);

  INFO("Conf", "SUCCESS");
}
