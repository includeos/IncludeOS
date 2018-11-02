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
#include <os>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <lest/lest.hpp>
#include <unistd.h>

#include <fstream>

const lest::test specification[] =
{
  {
    CASE("open() existing file for reading returns valid file descriptor")
    {
      int fd = open("/mnt/disk/file1", O_RDONLY);
      EXPECT(fd != -1);
      int res = close(fd);
      EXPECT(res != -1);
    }
  },
  {
    CASE("open() non-existing file for reading returns error, errno set to ENOENT")
    {
      int res = open("/mnt/disk/file666", O_RDONLY);
      EXPECT(res == -1);
      EXPECT(errno == ENOENT);
    }
  },
  {
    CASE("lseek() on open FD returns file offset")
    {
      int fd = open("/mnt/disk/file1", O_RDONLY);
      EXPECT(fd != -1);
      // get current offset
      off_t offset = lseek(fd, 0, SEEK_CUR);
      EXPECT(offset == 0);

      // seek from end
      struct stat sb;
      int res = stat("/mnt/disk/file1", &sb);
      EXPECT(res != -1);
      int size = sb.st_size;
      offset = lseek(fd, 0, SEEK_END);
      EXPECT(offset == size);

      // we can seek beyond end
      offset = lseek(fd, 100, SEEK_END);
      EXPECT(offset > size);

      // but not before beginning
      offset = lseek(fd, 0, SEEK_SET);
      EXPECT(offset == 0);
      offset = lseek(fd, -100, SEEK_CUR);
      EXPECT(offset == 0);

      res = close(fd);
      EXPECT(res != -1);
    }
  },
  {
    CASE("lseek() on non-open FD returns -1, errno set to EBADF")
    {
      int fd = open("/mnt/disk/file666", O_RDONLY);
      EXPECT(fd == -1);
      off_t offset = lseek(fd, 0, SEEK_SET);
      EXPECT(offset == -1);
      EXPECT(errno == EBADF);
    }
  },
  {
    CASE("lseek() with invalid whence returns -1, errno set to EINVAL")
    {
      int fd = open("/mnt/disk/file1", O_RDONLY);
      EXPECT(fd != -1);
      // valid values for whence are SEEK_SET, SEEK_CUR and SEEK_END
      off_t offset = lseek(fd, 0, SEEK_SET);
      EXPECT(offset != -1);
      offset = lseek(fd, 0, SEEK_CUR);
      EXPECT(offset != -1);
      offset = lseek(fd, 0, SEEK_END);
      EXPECT(offset != -1);
      // 42 is not a valid value for whence
      offset = lseek(fd, 0, 42);
      EXPECT(offset == -1);
      EXPECT(errno == EINVAL);
      int res = close(fd);
      EXPECT(res != -1);
    }
  },
  {
    CASE("read() reads requested number of bytes from open fd")
    {
      int fd = open("/mnt/disk/file1", O_RDONLY);
      EXPECT(fd != -1);

      struct stat sb;
      int res = stat("/mnt/disk/file1", &sb);
      EXPECT(res != -1);
      int size = sb.st_size;
      EXPECT(size > 0);

      char* buffer = (char*) malloc(1024);
      memset(buffer, 0, 1024);
      int bytes = read(fd, buffer, 1024);
      EXPECT(bytes == size);

      // verify that data from read are correct
      int cmp = strcmp(buffer, "content\n");
      EXPECT(cmp == 0);

      // test partial read, just first 4 bytes
      memset(buffer, 0, 1024);
      off_t offset = lseek(fd, 0, SEEK_SET);
      EXPECT(offset == 0);
      bytes = read(fd, buffer, 4);
      EXPECT(bytes == size - 4);
      cmp = strcmp(buffer, "content\n");
      EXPECT(cmp != 0);
      cmp = strcmp(buffer, "cont");
      EXPECT(cmp == 0);
      free(buffer);

      // read with NULL pointer fails, return EFAULT
      bytes = read(fd, nullptr, 404);
      EXPECT(bytes == -1);
      EXPECT(errno == EFAULT);

      res = close(fd);
      EXPECT(res == 0);
    }
  },
  {
    CASE("close() closes file desctiptors")
    {
      int fd = open("/mnt/disk/file1", O_RDONLY);
      EXPECT(fd != -1);
      int res = close(fd);
      EXPECT(res != -1);

      // check that fd is closed
      char* buffer = (char*) malloc(1024);
      memset(buffer, 0, 1024);
      int bytes = read(fd, buffer, 1024);
      EXPECT(bytes == -1);
      EXPECT(errno == EBADF);

      res = close(fd);
      EXPECT(res == -1);
    }
  },
  {
    CASE("file descriptors are independent")
    {
      // open two file descriptors for the same file
      int fd1 = open("/mnt/disk/file3", O_RDONLY);
      int fd2 = open("/mnt/disk/file3", O_RDONLY);
      EXPECT(fd1 != -1);
      EXPECT(fd2 != -1);
      // seek to different positions
      off_t offset1 = lseek(fd1, 2, SEEK_SET);
      off_t offset2 = lseek(fd2, 4, SEEK_SET);
      EXPECT(offset1 != offset2);

      char* buf1 = (char*) malloc(5);
      memset(buf1, 0, 5);
      char* buf2 = (char*) malloc(5);
      memset(buf2, 0, 5);
      // read 4 bytes from each file
      int bytes1 = read(fd1, buf1, 4);
      int bytes2 = read(fd2, buf2, 4);
      // verify that data read was different
      int cmp = strcmp(buf1, buf2);
      EXPECT(cmp != 0);

      // close one FD, lseek/read fails
      int res = close(fd2);
      EXPECT(res != -1);
      offset2 = lseek(fd2, 0, SEEK_SET);
      EXPECT(offset2 == -1);
      bytes2 = read(fd2, buf2, 4);
      EXPECT(bytes2 == -1);

      // can still read from file using the other FD
      bytes1 = read(fd1, buf1, 4);
      EXPECT(bytes1 != -1);
      EXPECT(bytes1 == 4);
      res = close(fd1);
      EXPECT(res != -1);
    }
  },
  {
    CASE("open() with nullptr path fails, errno is EFAULT")
    {
      int res = open(nullptr, O_RDONLY);
      EXPECT(res == -1);
      EXPECT(errno == EFAULT);
    }
  },
  {
    CASE("open() with empty string path fails, errno is ENOENT")
    {
      int res = open("", O_RDONLY);
      EXPECT(res == -1);
      EXPECT(errno == ENOENT);
    }
  },
  {
    CASE("fopen() opens a file") {
      FILE* f = fopen("/mnt/disk/file1", "r");
      EXPECT(f != nullptr);
      int res = fclose(f);
      EXPECT(res == 0);
    }
  },
  {
    CASE("fgetc() reads a character from an open file") {
      FILE* f = fopen("/mnt/disk/file1", "r");
      int c = fgetc(f);
      EXPECT(c != EOF);
      EXPECT(c == 'c'); // file contains the text "content\n"
      int res = fclose(f);
      EXPECT(res == 0);
    }
  },
  {
    CASE("fclose() closes an open file, preventing further reads") {
      FILE* f = fopen("/mnt/disk/file1", "r");
      int c = fgetc(f);
      EXPECT(c != EOF);
      int res = fclose(f);
      EXPECT(res == 0);
    }
  },
  {
    CASE("fgets() reads a character stream from an open file") {
      FILE* f = fopen("/mnt/disk/file3", "r");
      EXPECT(f != nullptr);
      char buf[256];
      memset(buf, 0, sizeof(buf));
      const char* ptr = fgets(buf, sizeof(buf), f);
      EXPECT(ptr != nullptr);
      int cmp = strncmp(buf, "even more content\n", sizeof(buf));
      EXPECT(cmp == 0);
      int res = fclose(f);
      EXPECT(res == 0);
    }
  },
  {
    CASE("fread() reads objects from an open file") {
      FILE* f = fopen("/mnt/disk/file1", "r");
      EXPECT(f != nullptr);
      char buf[256];
      memset(buf, 0, 256);
      // read first 7 chars from f
      size_t wanted_count {7};
      size_t count = fread(buf, sizeof(char), wanted_count, f);
      EXPECT(count == wanted_count);
      int cmp = strcmp(buf, "content");
      EXPECT(cmp == 0);
      int res = fclose(f);
      EXPECT(res == 0);
    }
  },
  {
    CASE("ifstream::read() reads a block of data from open stream") {
      std::ifstream is("/mnt/disk/file1");
      EXPECT_NOT(is.fail());
      char buf[256];
      memset(buf, 0, 256);
      is.read(buf, 256);
      EXPECT_NOT(is.bad()); // No errors
      EXPECT(is.eof()); // ... but we did get to the end of the file
      int cmp = strcmp(buf, "content\n");
      EXPECT(cmp == 0);
      is.close();
    }
  }
};

int main()
{
  INFO("POSIX file_fd", "Running tests for POSIX file_fd");

  auto failed = lest::run(specification, {"-p"});
  Expects(not failed);

  INFO("POSIX file_fd", "SUCCESS");
}
