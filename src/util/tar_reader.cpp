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

using namespace tar;

// -------------------- Element --------------------

long int Element::size() {
  // Size of element's size in bytes
  char* pEnd;
  long int filesize;
  filesize = strtol(header_.size, &pEnd, 8); // header_.size is octal
  return filesize;
}

// ----------------------- Tar -----------------------

const Element Tar::element(const std::string& path) const {
  for (auto element : elements_) {
    if (element.name() == path) {
      return element;
    }
  }

  throw Tar_exception(std::string{"Path " + path + " doesn't exist. Folder names should have a trailing /"});
}

std::vector<std::string> Tar::element_names() const {
  std::vector<std::string> element_names;

  for (auto element : elements_)
    element_names.push_back(element.name());

  return element_names;
}

// -------------------- Tar_reader --------------------

unsigned int Tar_reader::decompressed_length(const char* data, size_t size) {
  unsigned int dlen = data[size - 1];

  for (int i = 2; i <= 4; i++)
    dlen = DECOMPRESSION_SIZE*dlen + data[size - i];

  return dlen;
}

unsigned int Tar_reader::decompressed_length(Element& element) {
  // The original size of the tar.gz/compressed file is stored in the last 4 bytes of the file

  const char* last_blk = element.last_block(); // content of element's last block (512 bytes)

  long int size = element.size();
  long int remaining_size = size;

  // Getting the size of the actual content in the last block (isn't necessarily filled (512 bytes of data))
  // Are looking to find the position of the last 4 bytes of the file (where the original size of the tar.gz file is stored)
  if (size > SECTOR_SIZE)
    remaining_size = size - ((element.num_content_blocks() - 1) * SECTOR_SIZE);

  // Got what's left in last block in remaining_size

  // Main operation:

  unsigned int dlen = last_blk[remaining_size - 1];
/*
  const char* content = element.content();
  unsigned int dlen = content[element.size() - 1];
*/

  for (int i = 2; i <= 4; i++)
    dlen = DECOMPRESSION_SIZE*dlen + last_blk[remaining_size - i]; //content[element.size() - i];

  return dlen;
}

/*
unsigned int Tar_reader::decompressed_length(Element& element) {
  // The original size of the tar.gz/compressed file is stored in the last 4 bytes of the file

  const char* last_block = element.last_block()->block; // content of element's last block (512 bytes)

  long int size = element.size();
  long int remaining_size = size;

  // Getting the size of the actual content in the last block (isn't necessarily filled (512 bytes of data))
  // Are looking to find the position of the last 4 bytes of the file (where the original size of the tar.gz file is stored)
  if (size > SECTOR_SIZE)
    remaining_size = size - ((element.num_content_blocks() - 1) * SECTOR_SIZE);

  // Got what's left in last block in remaining_size

  // Main operation:

  unsigned int dlen = last_block[remaining_size - 1];

  for (int i = 2; i <= 4; i++)
    dlen = DECOMPRESSION_SIZE*dlen + last_block[remaining_size - i];

  return dlen;
}
*/

Tar& Tar_reader::read_uncompressed(const char* data, size_t size) {
  if (size == 0)
    throw Tar_exception("File is empty");

  if (size % SECTOR_SIZE not_eq 0)
    throw Tar_exception("Invalid size of tar file");

  // Go through the whole tar file block by block
  for (size_t i = 0; i < size; i += SECTOR_SIZE) {
    Tar_header* header = (Tar_header*) (data + i);
    Element element{*header};

    if (element.name().empty()) {
      // Empty header name - continue
      continue;
    }

    // Check if this is a directory or not (typeflag) (directories have no content)
    // If typeflag not set -> is not a directory and has content
    if (not element.typeflag_is_set() or not element.is_dir()) {
      i += SECTOR_SIZE; // move past header
      element.set_content_start(data + i);
      i += (SECTOR_SIZE * (element.num_content_blocks() - 1));  // move to the end of the element
    }

    tar_.add_element(element);
  }

  return tar_;
}

/*
Tar& Tar_reader::read_uncompressed(const char* data, size_t size) {
  if (size == 0)
    throw Tar_exception("File is empty");

  if (size % SECTOR_SIZE not_eq 0)
    throw Tar_exception("Invalid size of tar file");

  // Go through the whole tar file block by block
  for (size_t i = 0; i < size; i += SECTOR_SIZE) {
    Tar_header* header = (Tar_header*) (data + i);
    Element element{*header};

    if (element.name().empty()) {
      // Empty header name - continue
      continue;
    }

    // Check if this is a directory or not (typeflag) (directories have no content)

    // If typeflag not set (as with m files) -> is not a directory and has content
    if (not element.typeflag_is_set() or not element.is_dir()) {

      //element.set_content_start(file + i);
      //element.set_content_size(element.size());

// Comment out if pointer and size

      // Get the size of this file in the tarball
      long int filesize = element.size(); // Gives the size in bytes (converts from octal to decimal)

      int num_content_blocks = 0;
      if (filesize % SECTOR_SIZE not_eq 0)
        num_content_blocks = (filesize / SECTOR_SIZE) + 1;
      else
        num_content_blocks = (filesize / SECTOR_SIZE);

      for (int j = 0; j < num_content_blocks; j++) {
        i += SECTOR_SIZE; // Go to next block with content

        Content_block* content_blk = (Content_block*) (data + i);
        element.add_content_block(content_blk);
      }

    }

    tar_.add_element(element);
  }

  return tar_;
}
*/
