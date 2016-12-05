// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

#include <tar>

#include <iostream> // strtol

// -------------------- File_info --------------------

long int File_info::size() {
  // Size of file's content in bytes
  char* pEnd;
  long int filesize;
  filesize = strtol(header_.size, &pEnd, 8); // header_.size is octal
  return filesize;
}

// ----------------------- Tar -----------------------

const File_info Tar::file(const std::string& path) const {
  for (auto file_info : files_) {
    if (file_info.name() == path) {
      printf("Found file\n");

      return file_info;
    }
  }

  throw Tar_exception(std::string{"Path " + path + " doesn't exist"});
}

std::vector<std::string> Tar::file_names() const {
    std::vector<std::string> file_names;

    for (auto file_info : files_)
      file_names.push_back(file_info.name());

    return file_names;
  }

// -------------------- Tar_reader --------------------

Tar& Tar_reader::read(const char* file, size_t size) {
  printf("tar: const char* and size_t\n");

  if (size == 0)
    throw Tar_exception("File is empty");

  if (size % SECTOR_SIZE not_eq 0)
    throw Tar_exception("Invalid size of tar file");

  printf("-------------------------------------\n");
  printf("Read %u bytes from tar file\n", size);
  printf("So this tar file consists of %u sectors\n", size / SECTOR_SIZE);

  // Go through the whole buffer/tar file block by block
  for ( size_t i = 0; i < size; i += SECTOR_SIZE) {

    // One header or content at a time (one block)

    printf("----------------- New header -------------------\n");

    Tar_header* h = (Tar_header*) (file + i);
    File_info f{*h};

    if (strcmp(h->name, "") == 0) {
      printf("Empty header name: Continue\n");
      continue;
    }

    // if info.name() is a .tar.gz-file do something else (as is the case with mender):
    // decompress(.., ..);

    printf("Name: %s\n", f.name().c_str());
    printf("Mode: %s\n", f.mode().c_str());
    printf("UID: %s\n", f.uid().c_str());
    printf("GID: %s\n", f.gid().c_str());
    printf("Filesize: %ld\n", f.size());
    printf("Mod_time: %s\n", f.mod_time().c_str());
    printf("Checksum: %s\n", f.checksum().c_str());
    printf("Linkname: %s\n", f.linkname().c_str());
    printf("Magic: %s\n", f.magic().c_str());
    printf("Version: %s\n", f.version().c_str());
    printf("Uname: %s\n", f.uname().c_str());
    printf("Gname: %s\n", f.gname().c_str());
    printf("Devmajor: %s\n", f.devmajor().c_str());
    printf("Devminor: %s\n", f.devminor().c_str());
    printf("Prefix: %s\n", f.prefix().c_str());
    printf("Pad: %s\n", f.pad().c_str());
    printf("Typeflag: %c\n", f.typeflag());

    // Check if this is a directory or not (typeflag) (Directories have no content)

    // If typeflag not set (as with mender files) -> is not a directory and has content
    if (not f.typeflag_is_set() or not f.is_dir()) {  // Is not a directory so has content

      // f.set_start_index( (i / SECTOR_SIZE) - tar_.num_elements() );
      // Next block is first content block and we need to subtract number of headers (are incl. in i)

      // Get the size of this file in the tarball
      long int filesize = f.size(); // Gives the size in bytes (converts from octal to decimal)
      printf("Filesize decimal: %ld\n", filesize);

      int num_content_blocks = 0;
      if (filesize % SECTOR_SIZE not_eq 0)
        num_content_blocks = (filesize / SECTOR_SIZE) + 1;
      else
        num_content_blocks = (filesize / SECTOR_SIZE);
      printf("Num content blocks: %d\n", num_content_blocks);

      // f.set_num_content_blocks(num_content_blocks);

      for (int j = 0; j < num_content_blocks; j++) {
        i += SECTOR_SIZE; // Go to next block with content

        Content_block* content_blk = (Content_block*) (file + i);

        // printf("\nContent block:\n%.512s\n", content_blk->block);
        // NB: Doesn't stop at 512 without %.512s - writes the rest as well (until the file's end)

        //tar_.add_content_block(content_blk);
        f.add_content_block(content_blk);
      }
    }

    tar_.add_file(f);
  }

  return tar_;
}
