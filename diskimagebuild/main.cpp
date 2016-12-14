#define _XOPEN_SOURCE 1
#define _XOPEN_SOURCE_EXTENDED 1
#define _GNU_SOURCE 1

#include <cstdio>
#include <unistd.h>
#include <getopt.h>

#include "filetree.hpp"
FileSys fsys;

#define NO_ARGUMENT  0
#define REQ_ARGUMENT 1

void print_usage(char* const argv[])
{
  fprintf(stderr,
        "Usage:\t"
        "%s -v --file output.file [source folder]\n"
        "Note: The default source folder is the current directory\n"
        , argv[0]);
}

int main(int argc, char** argv)
{
  // put current directory in buffer used for default gathering folder
  char pwd_buffer[256];
  getcwd(pwd_buffer, sizeof(pwd_buffer));
  
  /// input folder
  std::string input_folder(pwd_buffer);
  /// output file
  std::string output_file;

  bool verbose = false;
  while (1)
  {
    int opt = getopt(argc, argv, "vf:");
    if (opt == -1) break;

    switch (opt) {
    case 'v':
       verbose = true;
       break;

    case 'f':
       output_file = std::string(optarg);
       break;

    default:
       //printf("Invalid option: %c\n", optopt);
       print_usage(argv);
       exit(EXIT_FAILURE);
    }
  }

  // remaining arguments (non-options)
  if (optind < argc)
  {
    input_folder = std::string(argv[optind]);
    if (verbose) {
      printf("Creating filesystem from: %s\n", input_folder.c_str());
    }
  }

  if (input_folder.empty() || output_file.empty())
  {
    print_usage(argv);
    return EXIT_FAILURE;
  }

  FILE* file = fopen(output_file.c_str(), "wb");
  chdir(input_folder.c_str());
  // walk filesystem subtree
  fsys.gather();

  // print filesystem contents recursively
  if (verbose)
    fsys.print();

  // write to disk image file
  long total = fsys.write(file);
  if (verbose) {
    printf("Written %ld bytes to %s\n", total, output_file.c_str());
  }
  fclose(file);
  
  return EXIT_SUCCESS;
}
