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

using namespace tar;

void print_header(const Element& element, const std::string& unique) {
  printf("%s - name of %s\n", element.name().c_str(), unique.c_str());
  printf("Mode of %s: %s\n", unique.c_str(), element.mode().c_str());
  printf("Uid of %s: %s\n", unique.c_str(), element.uid().c_str());
  printf("Gid of %s: %s\n", unique.c_str(), element.gid().c_str());
  printf("Size of %s: %ld\n", unique.c_str(), element.size());
  printf("Mod_time of %s: %s\n", unique.c_str(), element.mod_time().c_str());
  printf("Checksum of %s: %s\n", unique.c_str(), element.checksum().c_str());
  printf("Typeflag of %s: %c\n", unique.c_str(), element.typeflag());
  printf("Linkname of %s: %s\n", unique.c_str(), element.linkname().c_str());
  printf("Magic of %s: %s\n", unique.c_str(), element.magic().c_str());
  printf("Version of %s: %s\n", unique.c_str(), element.version().c_str());
  printf("Uname of %s: %s\n", unique.c_str(), element.uname().c_str());
  printf("Gname of %s: %s\n", unique.c_str(), element.gname().c_str());
  printf("Devmajor of %s: %s\n", unique.c_str(), element.devmajor().c_str());
  printf("Devminor of %s: %s\n", unique.c_str(), element.devminor().c_str());
  printf("Prefix of %s: %s\n", unique.c_str(), element.prefix().c_str());
  printf("Pad of %s: %s\n", unique.c_str(), element.pad().c_str());
}

void print_content(const Element& element, const std::string& unique) {
  if (element.is_dir()) {
    printf("%s is a folder\n", unique.c_str());
  } else {
    printf("%s is not a folder and has content:\n", unique.c_str());
    printf("First block of content: %.512s", element.content());
  }
}

void Service::start(const std::string&)
{
  const auto decompressed = Reader::decompress_tar(); // In CMakeLists.txt: set(TARFILE tar_example.tar.gz)
  Tar read_tarfile = Reader::read(decompressed.data(), decompressed.size());

  // Get the names of all the elements in the tarball
  auto found_elements = read_tarfile.element_names();
  for (auto name : found_elements)
    printf("%s - element\n", name.c_str());

  // Get a specific file in the tarball
  const std::string path1 = "tar_example/l1_f1/l2/README.md";
  auto& element = read_tarfile.element(path1);
  print_header(element, "README.md");
  print_content(element, "README.md");

  // Get a specific folder in the tarball
  // NB: No path match on folders if no trailing /
  const std::string path2 = "tar_example/l1_f1/l2/";
  auto& element2 = read_tarfile.element(path2);
  print_header(element2, "l2");
  print_content(element2, "l2");

  // Get all elements in the tarball
  auto& elements = read_tarfile.elements();
  for (auto& e : elements)
    printf("Name: %s Typeflag: %c\n", e.name().c_str(), e.typeflag());

  printf("Something special to close with\n");
}
