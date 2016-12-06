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

void print_header(Element& element) {
  printf("Tar header:\n");
  printf("Gotten name: %s\n", element.name().c_str());
  printf("Gotten mode: %s\n", element.mode().c_str());
  printf("Gotten uid: %s\n", element.uid().c_str());
  printf("Gotten gid: %s\n", element.gid().c_str());
  printf("Gotten size: %ld\n", element.size());
  printf("Gotten mod_time: %s\n", element.mod_time().c_str());
  printf("Gotten checksum: %s\n", element.checksum().c_str());
  printf("Gotten typeflag: %c\n", element.typeflag());
  printf("Gotten linkname: %s\n", element.linkname().c_str());
  printf("Gotten magic: %s\n", element.magic().c_str());
  printf("Gotten version: %s\n", element.version().c_str());
  printf("Gotten uname: %s\n", element.uname().c_str());
  printf("Gotten gname: %s\n", element.gname().c_str());
  printf("Gotten devmajor: %s\n", element.devmajor().c_str());
  printf("Gotten devminor: %s\n", element.devminor().c_str());
  printf("Gotten prefix: %s\n", element.prefix().c_str());
  printf("Gotten pad: %s\n", element.pad().c_str());
}

void print_content(Element& element) {
  if (element.is_dir()) {
    printf("This is a folder\n");
  } else {
    printf("This is not a folder and has content:\n");

    std::vector<Content_block*> result = element.content();
    for (Content_block* res : result)
      printf("Content block:\n%.512s\n", res->block);
  }
}

void Service::start(const std::string&)
{
  Tar_reader tr;
  Tar& read_tarfile = tr.read_binary_tar(); // Included .o file in CMakeLists.txt

  /* If compressed file the user could call:
  Tar& read_compressed_file = tr.decompress(file, size);  // tar.gz
  */

  std::vector<std::string> found_elements = read_tarfile.element_names();
  printf("Elements in tarball:\n");
  for (auto name : found_elements)
    printf("%s\n", name.c_str());

  // NB: No path match on folders if not trailing /

  /*std::string mender_path1 = "info";
  std::string mender_path2 = "header.tar.gz";*/

  std::string path1 = "tar_example_folder/level_1_folder_1/level_2_folder/README.md";
  std::string path2 = "tar_example_folder/level_1_folder_1/level_2_folder/";
  std::string path3 = "tar_example_folder/first_file.txt";
  std::string path4 = "tar_example_folder/level_1_folder_1/service.cpp";
  std::string path5 = "doesnt_exist.md";

  std::vector<Element> elements = read_tarfile.elements();
  for (auto e : elements) {
    if (e.name() == path1 or e.name() == path2 or e.name() == path3 or e.name() == path4) {
      print_header(e);
      print_content(e);
    }
  }

  // Other option (for each file you want):
  Element element = read_tarfile.element(path1);
  print_header(element);
  print_content(element);

  Element element2 = read_tarfile.element(path2);
  print_header(element2);
  print_content(element2);

  // Element element3 = read_tarfile.element(path5); // throws exception
}
