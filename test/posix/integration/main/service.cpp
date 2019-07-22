
#include <cstdio>
#include <string>
#include <cassert>

int main(int argc, char** argv)
{
  printf("Argc: %d\n", argc);

  for (int i = 0; i < argc; i++)
    printf("Arg %i: %s\n", i, argv[i]);

  //We are execucting out of tree  assert(std::string(argv[0]) == "service");
  //we could perhaps verify that it ends with posix_main instead
  assert(std::string(argv[1]) == "booted");
  assert(std::string(argv[2]) == "with");
  assert(std::string(argv[3]) == "vmrunner");

  // We want to veirify this "exit status" on the back-end
  return 200;
}
