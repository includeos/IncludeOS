// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

#include <service>

#include <tar>

void print_header(File_info& file) {
  printf("Tar header:\n");
  printf("Gotten name: %s\n", file.name().c_str());
  printf("Gotten mode: %s\n", file.mode().c_str());
  printf("Gotten uid: %s\n", file.uid().c_str());
  printf("Gotten gid: %s\n", file.gid().c_str());
  printf("Gotten size: %ld\n", file.size());
  printf("Gotten mod_time: %s\n", file.mod_time().c_str());
  printf("Gotten checksum: %s\n", file.checksum().c_str());
  printf("Gotten typeflag: %c\n", file.typeflag());
  printf("Gotten linkname: %s\n", file.linkname().c_str());
  printf("Gotten magic: %s\n", file.magic().c_str());
  printf("Gotten version: %s\n", file.version().c_str());
  printf("Gotten uname: %s\n", file.uname().c_str());
  printf("Gotten gname: %s\n", file.gname().c_str());
  printf("Gotten devmajor: %s\n", file.devmajor().c_str());
  printf("Gotten devminor: %s\n", file.devminor().c_str());
  printf("Gotten prefix: %s\n", file.prefix().c_str());
  printf("Gotten pad: %s\n", file.pad().c_str());
}

void print_content(File_info& file) {
  if (file.is_dir()) {
    printf("This is a folder\n");
  } else {
    printf("This is not a folder and has content:\n");

    std::vector<Content_block*> result = file.content();
    for (Content_block* res : result)
      printf("Content block:\n%.512s\n", res->block);
  }
}

void Service::start(const std::string&)
{
  Tar_reader tr;
  Tar& read_tarfile = tr.read_binary_tar(); // Included .o file in CMakeLists.txt
  // Tar& read_tarfile = tr.read(file_content, size);  // tar and mender

  /* If compressed file the user could call:
  Tar& read_compressed_file = tr.decompress(file, size);  // tar.gz
  */

  std::vector<std::string> found_files = read_tarfile.file_names();
  printf("Files in tarball:\n");
  for (auto name : found_files)
    printf("%s\n", name.c_str());

  // NB: No path match on folders if not trailing /

  /*std::string mender_filename1 = "info";
  std::string mender_filename2 = "header.tar.gz";*/

  std::string filename1 = "tar_example_folder/level_1_folder_1/level_2_folder/README.md";
  std::string filename2 = "tar_example_folder/level_1_folder_1/level_2_folder/";
  std::string filename3 = "tar_example_folder/first_file.txt";
  std::string filename4 = "tar_example_folder/level_1_folder_1/service.cpp";
  std::string filename5 = "doesnt_exist.md";

  std::vector<File_info> files = read_tarfile.files();
  for (auto f : files) {
    if (f.name() == filename1 or f.name() == filename2 or f.name() == filename3
      or f.name() == filename4) {
      print_header(f);
      print_content(f);
    }
  }

  // Other option (for each file you want):
  File_info file = read_tarfile.file(filename1);
  print_header(file);
  print_content(file);

  File_info file2 = read_tarfile.file(filename2);
  print_header(file2);
  print_content(file2);

  // File_info file3 = read_tarfile.file(filename5); // throws exception

  //tr.read("artifact.mender");
  //tr.read("tar_example_folder.tar");
  //tr.read("tar_ex_2.tar");

  //tr.read("multiple_folders_no_virtio.tar");

  //tr.read("");

  //tr.read("level_1_folder_1.tar.gz");
}
