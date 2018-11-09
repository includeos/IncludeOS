#define _XOPEN_SOURCE 1
#define _XOPEN_SOURCE_EXTENDED 1
#define _GNU_SOURCE 1

#include <cstdio>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>

#include "filetree.hpp"
FileSys fsys;

#define NO_ARGUMENT  0
#define REQ_ARGUMENT 1

void print_usage(char* const argv[])
{
  fprintf(stderr,
        "Creates a minimal read-only FAT file image from given source folder\n"
        "Usage:\n"
        "\t%s -v -o file [source folder]\n"
        "Options:\n"
        "\t-v\t\tVerbose output\n"
        "\t-o <file>\tPlace the output into <file>\n"
        "\n"
        "Note: Source folder defaults to the current directory\n"
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
    int opt = getopt(argc, argv, "vo:");
    if (opt == -1) break;

    switch (opt) {
    case 'v':
       verbose = true;
       break;

    case 'o':
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
  int res = chdir(input_folder.c_str());
  if (res < 0)
  {
    fprintf(stderr, "Disk builder failed to enter folder '%s'!\n",
            input_folder.c_str());
    fprintf(stderr, "Make corrections and try again\n");
    exit(1);
  }
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
