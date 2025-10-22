// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

#include <common.cxx>
#include <tar>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

CASE("Reading single entry tar file")
{
  tar::Reader r;
  struct stat st;
  int res = stat("test-single.tar", &st);
  EXPECT(res != -1);
  size_t size = st.st_size;
  int fd = open("test-single.tar", O_RDONLY);
  EXPECT_NOT(fd == -1);
  const uint8_t *mem = (const uint8_t *)mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
  tar::Tar tar = r.read_uncompressed(mem, size);
  EXPECT(tar.num_elements() == 1);
  auto names = tar.element_names();
  EXPECT(names.size() == 1);
  const auto& elements = tar.elements();
  EXPECT(elements.size() == 1);
  const auto& e = elements.at(0);
  EXPECT_NOT(e.is_dir());
  EXPECT(e.typeflag_is_set());
  EXPECT(e.typeflag() == REGTYPE); // regular file
  EXPECT_NOT(e.is_empty());
  EXPECT(e.is_ustar());
  EXPECT_NOT(e.is_tar_gz());
  EXPECT(e.num_content_blocks() > 0);
  EXPECT(e.size() > 0);
  const auto& name = e.name();
  EXPECT(name.find("CMakeLists.txt") != std::string::npos);
  EXPECT_NO_THROW(auto& only_element = tar.element("../CMakeLists.txt"));
  EXPECT_THROWS_AS(auto& missing_element = tar.element("not_there.txt"), tar::Tar_exception);

  // try to decompress a non-tar.gz element
  EXPECT_THROWS_AS(tar::Tar inner_tar = r.decompress(e), tar::Tar_exception);
  close(fd);
}

CASE("Reading multiple entry tar file")
{
  tar::Reader r;
  struct stat st;
  int res = stat("test-multiple.tar", &st);
  EXPECT(res != -1);
  size_t size = st.st_size;
  int fd = open("test-multiple.tar", O_RDONLY);
  EXPECT_NOT(fd == -1);
  const uint8_t *mem = (const uint8_t *)mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
  tar::Tar tar = r.read_uncompressed(mem, size);
  auto names = tar.element_names();
  EXPECT(names.size() > 1);
  const auto& elements = tar.elements();
  EXPECT(elements.size() > 1);
  const auto& e = elements.at(elements.size() - 1);
  EXPECT(e.name().find(".py") != std::string::npos);
  close(fd);
}

CASE("Reading invalid tar file")
{
  tar::Reader r;
  struct stat st;
  int res = stat("test-invalid.tar", &st);
  EXPECT(res != -1);
  size_t size = st.st_size;
  int fd = open("test-invalid.tar", O_RDONLY);
  EXPECT_NOT(fd == -1);
  const uint8_t *mem = (const uint8_t *)mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
  EXPECT_THROWS_AS(tar::Tar tar = r.read_uncompressed(mem, size), tar::Tar_exception);
  close(fd);
}

CASE("Reading corrupt tar.gz file")
{
  tar::Reader r;
  struct stat st;
  int res = stat("test-corrupt.gz", &st);
  EXPECT(res != -1);
  size_t size = st.st_size;
  int fd = open("test-corrupt.gz", O_RDONLY);
  EXPECT_NOT(fd == -1);
  const uint8_t *mem = (const uint8_t *)mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
  EXPECT_THROWS_AS(tar::Tar tar = r.decompress(mem, size), tar::Tar_exception);
  close(fd);
}

CASE("Reading tar.gz inside tar file")
{
  tar::Reader r;
  struct stat st;
  int res = stat("test-tar-gz-inside.tar", &st);
  EXPECT(res != -1);
  size_t size = st.st_size;
  int fd = open("test-tar-gz-inside.tar", O_RDONLY);
  EXPECT_NOT(fd == -1);
  const uint8_t *mem = (const uint8_t *)mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
  tar::Tar tar = r.read_uncompressed(mem, size);
  EXPECT(tar.num_elements() == 1);
  const auto& e = tar.elements().at(0);
  EXPECT(e.is_tar_gz());
  EXPECT(e.name().find(".tar.gz") != std::string::npos);
  tar::Tar inner_tar = r.decompress(e);
  EXPECT(inner_tar.num_elements() == 1);
  const auto& inner_e = inner_tar.elements().at(0);
  EXPECT(inner_e.name().find(".tar.gz") == std::string::npos);
  close(fd);
}
